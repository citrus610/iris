#include "nnue.h"

namespace nnue
{

#ifndef NNUE
#define NNUE ""
#endif

INCBIN(nnue_raw, NNUE);

usize Feature::get_index(i8 color)
{
    const usize index_color = piece::get_color(this->piece) != color;
    const usize index_piece = piece::get_type(this->piece);
    const usize index_square = square::get_relative(this->square, color);

    return usize(384) * index_color + usize(64) * index_piece + index_square;
};

void Accumulator::clear()
{
    for (i8 color = 0; color < 2; ++color) {
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->data[color][i] = PARAMS.in_biases[i];
        }
    }
};

void Accumulator::refresh(Board& board)
{
    this->clear();

    auto features = arrayvec<Feature, 32>();
    auto occupied = board.get_occupied();

    while (occupied)
    {
        const auto square = bitboard::pop_lsb(occupied);
        const auto piece = board.get_piece_at(square);

        features.add(Feature {
            .piece = piece,
            .square = square
        });
    }

    for (i8 color = 0; color < 2; ++color) {
        for (auto& feature : features) {
            for (usize i = 0; i < size::HIDDEN; ++i) {
                this->data[color][i] += PARAMS.in_weights[feature.get_index(color)][i];
            }
        }
    }
};

void Accumulator::update(const Accumulator& parent, i8 color)
{
    const auto adds_count = this->updates.adds.size();
    const auto subs_count = this->updates.subs.size();

    usize adds[2];
    usize subs[2];

    if (adds_count == 2 && subs_count == 2) {
        adds[0] = this->updates.adds[0].get_index(color);
        adds[1] = this->updates.adds[1].get_index(color);
        subs[0] = this->updates.subs[0].get_index(color);
        subs[1] = this->updates.subs[1].get_index(color);

        this->edit<2, 2>(parent, adds, subs, color);
    }
    else if (adds_count == 1 && subs_count == 2) {
        adds[0] = this->updates.adds[0].get_index(color);
        subs[0] = this->updates.subs[0].get_index(color);
        subs[1] = this->updates.subs[1].get_index(color);

        this->edit<1, 2>(parent, adds, subs, color);
    }
    else if (adds_count == 1 && subs_count == 1) {
        adds[0] = this->updates.adds[0].get_index(color);
        subs[0] = this->updates.subs[0].get_index(color);

        this->edit<1, 1>(parent, adds, subs, color);
    }
    else {
        assert(false);
    }
};

template <usize ADD, usize SUB>
void Accumulator::edit(const Accumulator& parent, usize adds[], usize subs[], i8 color)
{
    auto in = parent.data[color];
    auto out = this->data[color];

    i16* adds_vec[ADD];
    i16* subs_vec[SUB];

    for (usize i = 0; i < ADD; ++i) {
        adds_vec[i] = PARAMS.in_weights[adds[i]];
    }

    for (usize i = 0; i < SUB; ++i) {
        subs_vec[i] = PARAMS.in_weights[subs[i]];
    }

    #ifdef __AVX2__
        for (usize i = 0; i < size::HIDDEN; i += 16) {
            auto vec = _mm256_load_si256((const __m256i*)&in[i]);

            for (usize k = 0; k < ADD; ++k) {
                vec = _mm256_add_epi16(vec, _mm256_load_si256((const __m256i*)&adds_vec[k][i]));
            }

            for (usize k = 0; k < SUB; ++k) {
                vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&subs_vec[k][i]));
            }

            _mm256_store_si256((__m256i*)&out[i], vec);
        }
    #else
        for (usize i = 0; i < size::HIDDEN; ++i) {
            out[i] = in[i];

            for (usize k = 0; k < ADD; ++k) {
                out[i] += adds_vec[k][i];
            }

            for (usize k = 0; k < SUB; ++k) {
                out[i] -= subs_vec[k][i];
            }
        }
    #endif
};

Net::Net()
{
    this->index = 0;
};

i32 Net::get_eval(i8 color)
{
    i32 score = 0;

    #ifdef __AVX2__
        // Adds the stm vec and nstm vec
        auto vec = _mm256_add_epi32(
            nnue::get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[color], PARAMS.out_weights[0]),
            nnue::get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[!color], PARAMS.out_weights[1])
        );

        // Adds the 2 128-bit lanes
        auto sum = _mm_add_epi32(
            _mm256_extracti128_si256(vec, 0),
            _mm256_extracti128_si256(vec, 1)
        );

        // Does horizontal addition twice
        sum = _mm_hadd_epi32(sum, sum);
        sum = _mm_hadd_epi32(sum, sum);

        // Extracts the result
        score = _mm_cvtsi128_si32(sum);
    #else
        score += nnue::get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[color], PARAMS.out_weights[0]);
        score += nnue::get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[!color], PARAMS.out_weights[1]);
    #endif

    return (score / scale::L0 + PARAMS.out_bias) * scale::EVAL / (scale::L0 * scale::L1);
};

void Net::refresh(Board& board)
{
    this->stack[this->index].refresh(board);
};

void Net::make(Board& board, const u16& move)
{
    // Adds to stack
    this->index += 1;

    // Clears updates
    auto& adds = this->stack[this->index].updates.adds;
    auto& subs = this->stack[this->index].updates.subs;

    adds.clear();
    subs.clear();

    // Gets move data
    const auto move_type = move::get_type(move);
    const auto from = move::get_from(move);
    const auto to = move::get_to(move);

    const auto piece = board.get_piece_at(from);
    const auto color = board.get_color();
    const auto captured = move_type == move::type::CASTLING ? i8(piece::NONE) : board.get_piece_at(to);

    // Checks move type
    if (move_type == move::type::CASTLING) {
        bool castle_short = to > from;

        const auto king_to = castling::get_king_to(color, castle_short);
        const auto rook_to = castling::get_rook_to(color, castle_short);

        adds.add(Feature { .piece = piece::create(piece::type::KING, color), .square = king_to });
        adds.add(Feature { .piece = piece::create(piece::type::ROOK, color), .square = rook_to });
        subs.add(Feature { .piece = piece::create(piece::type::KING, color), .square = from });
        subs.add(Feature { .piece = piece::create(piece::type::ROOK, color), .square = to });
    }
    else if (move_type == move::type::PROMOTION) {
        adds.add(Feature { .piece = piece::create(move::get_promotion_type(move), color), .square = to });
        subs.add(Feature { .piece = piece, .square = from });
    }
    else {
        adds.add(Feature { .piece = piece, .square = to });
        subs.add(Feature { .piece = piece, .square = from });
    }

    // Checks capture
    if (captured != piece::NONE) {
        subs.add(Feature { .piece = captured, .square = to });
    }
    else if (move_type == move::type::ENPASSANT) {
        subs.add(Feature { .piece = piece::create(piece::type::PAWN, !color), .square = i8(to ^ 8) });
    }

    // Updates
    for (i8 color = 0; color < 2; ++color) {
        this->stack[this->index].update(this->stack[this->index - 1], color);
    }
};

void Net::unmake()
{
    this->index -= 1;
};

void init()
{
    std::memcpy((void*)&PARAMS, nnue_raw_data, sizeof(PARAMS));
};

};