#include "nnue.h"

namespace nnue
{

#ifndef NNUE
#define NNUE ""
#endif

INCBIN(nnue_raw, NNUE);

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

    auto occupied = board.get_occupied();

    while (occupied)
    {
        const auto square = bitboard::pop_lsb(occupied);
        const auto color = board.get_color_at(square);
        const auto type = board.get_type_at(square);

        const auto index_white = nnue::get_index(color, type, square, color::WHITE);
        const auto index_black = nnue::get_index(color, type, square, color::BLACK);

        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->data[0][i] += PARAMS.in_weights[index_white][i];
            this->data[1][i] += PARAMS.in_weights[index_black][i];
        }
    }
};

void Accumulator::make(const Accumulator& parent, i8 color)
{
    // Gets move data
    const auto move_type = move::get_type(this->update.move);
    const auto from = move::get_from(this->update.move);
    const auto to = move::get_to(this->update.move);

    const auto piece_type = piece::get_type(this->update.piece);
    const auto piece_color = piece::get_color(this->update.piece);

    // Checks castling
    if (move_type == move::type::CASTLING) {
        const bool castle_short = to > from;
        const auto king_to = castling::get_king_to(piece_color, castle_short);
        const auto rook_to = castling::get_rook_to(piece_color, castle_short);

        this->edit_add2_sub2(
            parent,
            nnue::get_index(piece_color, piece::type::KING, king_to, color),
            nnue::get_index(piece_color, piece::type::ROOK, rook_to , color),
            nnue::get_index(piece_color, piece::type::KING, from, color),
            nnue::get_index(piece_color, piece::type::ROOK, to, color),
            color
        );

        return;
    }

    // Checks moving piece
    const auto add1 = nnue::get_index(piece_color, move_type == move::type::PROMOTION ? move::get_promotion_type(this->update.move) : piece_type, to, color);
    const auto sub1 = nnue::get_index(piece_color, piece_type, from, color);

    // Checks capture
    if (this->update.captured != piece::NONE) {
        this->edit_add1_sub2(
            parent,
            add1,
            sub1,
            nnue::get_index(!piece_color, piece::get_type(this->update.captured), to, color),
            color
        );

        return;
    }

    // Checks enpassant
    if (move_type == move::type::ENPASSANT) {
        this->edit_add1_sub2(
            parent,
            add1,
            sub1,
            nnue::get_index(!piece_color, piece::type::PAWN, to ^ 8, color),
            color
        );

        return;
    }
    
    this->edit_add1_sub1(
        parent,
        add1,
        sub1,
        color
    );
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
        if (this->stack[this->index].is_updated[color]) {
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
    this->stack[this->index].is_updated[color::WHITE] = true;
    this->stack[this->index].is_updated[color::BLACK] = true;
};

void Net::update(i8 color)
{
    usize start = this->index - 1;

    while (true)
    {
        if (this->stack[start].is_updated[color]) {
            break;
        }

        start -= 1;
    }

    for (usize i = start; i < this->index; ++i) {
        this->stack[i + 1].make(this->stack[i], color);
        this->stack[i + 1].is_updated[color] = true;
    }
};

void Net::make(Board& board, const u16& move)
{
    this->index += 1;

    this->stack[this->index].update.move = move;
    this->stack[this->index].update.piece = board.get_piece_at(move::get_from(move));
    this->stack[this->index].update.captured = board.get_piece_at(move::get_to(move));

    this->stack[this->index].is_updated[color::WHITE] = false;
    this->stack[this->index].is_updated[color::BLACK] = false;
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