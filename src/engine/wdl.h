#pragma once

#include "eval.h"

namespace wdl
{

constexpr i32 PAWN_VALUE_NORMALIZED = 100;
constexpr f64 A[] = { 161.09824993, -431.48193676, 321.58846951, 112.83569357 };
constexpr f64 B[] = { 42.14414753, -95.37235696, 114.89052735, 14.85352801 };

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