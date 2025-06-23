#include "eval.h"

namespace eval
{

i32 get(Board& board, nnue::Net& nnue)
{
    // Gets score from nnue
    i32 score = nnue.get_eval(board.get_color());

    // Scales score based on material
    i32 scale = 0;

    scale += bitboard::get_count(board.get_pieces(piece::type::PAWN)) * SCALE_PAWN;
    scale += bitboard::get_count(board.get_pieces(piece::type::KNIGHT)) * SCALE_KNIGHT;
    scale += bitboard::get_count(board.get_pieces(piece::type::BISHOP)) * SCALE_BISHOP;
    scale += bitboard::get_count(board.get_pieces(piece::type::ROOK)) * SCALE_ROOK;
    scale += bitboard::get_count(board.get_pieces(piece::type::QUEEN)) * SCALE_QUEEN;

    score = score * (scale + SCALE_MIN) / SCALE_MAX;

    // Clamps score
    return std::clamp(score, -score::MATE_FOUND + 1, score::MATE_FOUND - 1);
};

i32 get_adjusted(i32 eval, i32 correction, i32 halfmove)
{
    return std::clamp(eval * (200 - halfmove) / 200 + correction, -score::MATE_FOUND + 1, score::MATE_FOUND - 1);
};

};