#include "nnue.h"

namespace nnue
{

#ifndef NNUE
#define NNUE ""
#endif

INCBIN(nnue_raw, NNUE);

Net::Net()
{
    this->index = 0;
    this->clear();
};

i32 Net::get_eval(i8 color)
{
    i32 score = 0;

    #ifdef __AVX2__
        // Adds the stm vec and nstm vec
        auto vec = _mm256_add_epi32(
            get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[color], PARAMS.out_weights[0]),
            get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[!color], PARAMS.out_weights[1])
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
        score += get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[color], PARAMS.out_weights[0]);
        score += get_linear<size::HIDDEN, scale::L0>(this->stack[this->index].data[!color], PARAMS.out_weights[1]);
    #endif

    return (score / scale::L0 + PARAMS.out_bias) * scale::EVAL / (scale::L0 * scale::L1);
};

template <bool ADD>
void Net::update(i8 color, i8 type, i8 square)
{
    const auto [index_white, index_black] = get_index(color, type, square);

    for (usize i = 0; i < size::HIDDEN; ++i) {
        if constexpr (ADD) {
            this->stack[this->index].data[0][i] += PARAMS.in_weights[index_white][i];
            this->stack[this->index].data[1][i] += PARAMS.in_weights[index_black][i];
        }
        else {
            this->stack[this->index].data[0][i] -= PARAMS.in_weights[index_white][i];
            this->stack[this->index].data[1][i] -= PARAMS.in_weights[index_black][i];
        }
    }
};

void Net::clear()
{
    for (i8 color = 0; color < 2; ++color) {
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->stack[this->index].data[color][i] = PARAMS.in_biases[i];
        }
    }
};

void Net::refresh(Board& board)
{
    this->clear();

    for (i8 sq = 0; sq < 64; ++sq) {
        const i8 piece = board.get_piece_at(sq);

        if (piece == piece::NONE) {
            continue;
        }

        const i8 type = piece::get_type(piece);
        const i8 color = piece::get_color(piece);

        this->update<true>(color, type, sq);
    }
};

void Net::make(Board& board, const u16& move)
{
    // Pushes to stack
    this->index += 1;
    this->stack[this->index] = this->stack[this->index - 1];

    // Gets move data
    const auto move_type = move::get_type(move);
    const auto from = move::get_from(move);
    const auto to = move::get_to(move);

    const auto piece = board.get_piece_at(from);
    const auto type = piece::get_type(piece);
    const auto color = piece::get_color(piece);
    
    assert(piece != piece::NONE);
    assert(color == board.get_color());

    // Checks capture
    const auto captured = move_type == move::type::CASTLING ? piece::type::NONE : board.get_type_at(to);

    if (captured != piece::type::NONE) {
        this->update<false>(!color, captured, to);
    }

    // Checks move type
    if (move_type == move::type::CASTLING) {
        assert(type == piece::type::KING);

        bool castle_short = to > from;

        const auto king_to = castling::get_king_to(color, castle_short);
        const auto rook_to = castling::get_rook_to(color, castle_short);

        this->update<false>(color, piece::type::KING, from);
        this->update<false>(color, piece::type::ROOK, to);

        this->update<true>(color, piece::type::KING, king_to);
        this->update<true>(color, piece::type::ROOK, rook_to);
    }
    else if (move_type == move::type::PROMOTION) {
        assert(type == piece::type::PAWN);

        const auto promotion = move::get_promotion_type(move);

        this->update<false>(color, type, from);
        this->update<true>(color, promotion, to);
    }
    else {
        this->update<false>(color, type, from);
        this->update<true>(color, type, to);
    }

    // Removes enpassant pawn
    if (move_type == move::type::ENPASSANT) {
        assert(type == piece::type::PAWN);

        this->update<false>(!color, piece::type::PAWN, to ^ 8);
    }
};

void Net::unmake()
{
    assert(this->index > 0);

    this->index -= 1;
};

void init()
{
    std::memcpy((void*)&PARAMS, nnue_raw_data, sizeof(PARAMS));
};

template void Net::update<true>(i8, i8, i8);
template void Net::update<false>(i8, i8, i8);

};