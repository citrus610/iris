#include "eval.h"

namespace eval
{

i32 get(Board& board, nnue::Net& nnue)
{
    // Gets score from nnue
    i32 score = nnue.get_eval(board.get_color());

    // Clamps score
    return std::clamp(score, -score::MATE_FOUND + 1, score::MATE_FOUND - 1);
};

i32 get_adjusted(i32 eval, i32 correction, i32 halfmove)
{
    return std::clamp(eval * (200 - halfmove) / 200 + correction, -score::MATE_FOUND + 1, score::MATE_FOUND - 1);
};

};