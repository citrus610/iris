#pragma once

#include "eval.h"

namespace wdl
{

constexpr i32 PAWN_VALUE_NORMALIZED = 178;
constexpr f64 A[] = { 101.45431284, -249.78262615, 121.56304392, 204.70137718 };
constexpr f64 B[] = { 94.97203972, -250.19908287, 242.41961263, -9.38316158 };

inline i32 get_material(Board& board)
{
    const i32 pawns = bitboard::get_count(board.get_pieces(piece::type::PAWN));
    const i32 knights = bitboard::get_count(board.get_pieces(piece::type::KNIGHT));
    const i32 bishops = bitboard::get_count(board.get_pieces(piece::type::BISHOP));
    const i32 rooks = bitboard::get_count(board.get_pieces(piece::type::ROOK));
    const i32 queens = bitboard::get_count(board.get_pieces(piece::type::QUEEN));

    return pawns + 3 * knights + 3 * bishops + 5 * rooks + 9 * queens;
};

inline i32 get_score_normalized(i32 score, i32 material)
{
    if (std::abs(score) < 2 || std::abs(score) >= eval::score::MATE_FOUND) {
        return score;
    }

    const f64 x = double(std::clamp(material, 16, 78)) / 58.0;
    const f64 a = ((A[0] * x + A[1]) * x + A[2]) * x + A[3];

    return static_cast<i32>(std::round(100.0 * static_cast<f64>(score) / a));
};

};