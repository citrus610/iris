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

void Accumulator::make(const Accumulator& parent, i8 color)
{
    const auto adds = this->update.adds.size();
    const auto subs = this->update.subs.size();

    if (adds == 2 && subs == 2) {
        this->edit_add2_sub2(
            parent,
            this->update.adds[0].get_index(color),
            this->update.adds[1].get_index(color),
            this->update.subs[0].get_index(color),
            this->update.subs[1].get_index(color),
            color
        );
    }
    else if (adds == 1 && subs == 2) {
        this->edit_add1_sub2(
            parent,
            this->update.adds[0].get_index(color),
            this->update.subs[0].get_index(color),
            this->update.subs[1].get_index(color),
            color
        );
    }
    else if (adds == 1 && subs == 1) {
        this->edit_add1_sub1(
            parent,
            this->update.adds[0].get_index(color),
            this->update.subs[0].get_index(color),
            color
        );
    }
    else {
        assert(false);
    }

    this->update.is_updated[color] = true;
};

void Accumulator::edit_add1_sub1(const Accumulator& parent, usize add1, usize sub1, i8 color)
{
    const auto vec_add1 = PARAMS.in_weights[add1];
    const auto vec_sub1 = PARAMS.in_weights[sub1];

    #ifdef __AVX2__
        for (usize i = 0; i < size::HIDDEN; i += 16) {
            auto vec = _mm256_load_si256((const __m256i*)&parent.data[color][i]);

            vec = _mm256_add_epi16(vec, _mm256_load_si256((const __m256i*)&vec_add1[i]));
            vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&vec_sub1[i]));

            _mm256_store_si256((__m256i*)&this->data[color][i], vec);
        }
    #else
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->data[color][i] = parent.data[color][i] + vec_add1[i] - vec_sub1[i];
        }
    #endif
};

void Accumulator::edit_add1_sub2(const Accumulator& parent, usize add1, usize sub1, usize sub2, i8 color)
{
    const auto vec_add1 = PARAMS.in_weights[add1];
    const auto vec_sub1 = PARAMS.in_weights[sub1];
    const auto vec_sub2 = PARAMS.in_weights[sub2];

    #ifdef __AVX2__
        for (usize i = 0; i < size::HIDDEN; i += 16) {
            auto vec = _mm256_load_si256((const __m256i*)&parent.data[color][i]);

            vec = _mm256_add_epi16(vec, _mm256_load_si256((const __m256i*)&vec_add1[i]));
            vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&vec_sub1[i]));
            vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&vec_sub2[i]));

            _mm256_store_si256((__m256i*)&this->data[color][i], vec);
        }
    #else
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->data[color][i] = parent.data[color][i] + vec_add1[i] - vec_sub1[i] - vec_sub2[i];
        }
    #endif
};

void Accumulator::edit_add2_sub2(const Accumulator& parent, usize add1, usize add2, usize sub1, usize sub2, i8 color)
{
    const auto vec_add1 = PARAMS.in_weights[add1];
    const auto vec_add2 = PARAMS.in_weights[add2];
    const auto vec_sub1 = PARAMS.in_weights[sub1];
    const auto vec_sub2 = PARAMS.in_weights[sub2];

    #ifdef __AVX2__
        for (usize i = 0; i < size::HIDDEN; i += 16) {
            auto vec = _mm256_load_si256((const __m256i*)&parent.data[color][i]);

            vec = _mm256_add_epi16(vec, _mm256_load_si256((const __m256i*)&vec_add1[i]));
            vec = _mm256_add_epi16(vec, _mm256_load_si256((const __m256i*)&vec_add2[i]));
            vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&vec_sub1[i]));
            vec = _mm256_sub_epi16(vec, _mm256_load_si256((const __m256i*)&vec_sub2[i]));

            _mm256_store_si256((__m256i*)&this->data[color][i], vec);
        }
    #else
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->data[color][i] = parent.data[color][i] + vec_add1[i] + vec_add2[i] - vec_sub1[i] - vec_sub2[i];
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

    for (i8 color = 0; color < 2; ++color) {
        if (this->stack[this->index].update.is_updated[color]) {
            continue;
        }

        this->update(color);
    }

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

void Net::update(i8 color)
{
    usize start = this->index;

    while (start > 1)
    {
        if (this->stack[start - 1].update.is_updated[color]) {
            break;
        }

        start -= 1;
    }

    for (usize i = start; i <= this->index; ++i) {
        this->stack[i].make(this->stack[i - 1], color);
    }
};

void Net::make(Board& board, const u16& move)
{
    // Adds to stack
    this->index += 1;

    // Clears updates
    auto& adds = this->stack[this->index].update.adds;
    auto& subs = this->stack[this->index].update.subs;

    adds.clear();
    subs.clear();

    this->stack[this->index].update.is_updated[color::WHITE] = false;
    this->stack[this->index].update.is_updated[color::BLACK] = false;

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