#pragma once

#include "../chess/chess.h"
#include "nnue.h"

namespace eval::score
{

constexpr i32 DRAW = 0;
constexpr i32 MATE = 32000 + MAX_PLY;
constexpr i32 MATE_FOUND = MATE - MAX_PLY;
constexpr i32 NONE = MATE + 1;
constexpr i32 INFINITE = INT16_MAX;

};

namespace eval
{

constexpr i32 PIECE_VALUE[6] = {
    100, 320, 330, 500, 900, 100000
};

constexpr i32 SCALE_PAWN = 1;
constexpr i32 SCALE_KNIGHT = 3;
constexpr i32 SCALE_BISHOP = 3;
constexpr i32 SCALE_ROOK = 5;
constexpr i32 SCALE_QUEEN = 10;

constexpr i32 SCALE_MAX = 256;
constexpr i32 SCALE_MIN = SCALE_MAX - SCALE_PAWN * 16 - SCALE_KNIGHT * 4 - SCALE_BISHOP * 4 - SCALE_ROOK * 4 - SCALE_QUEEN * 2;

i32 get(Board& board, nnue::Net& nnue);

i32 get_adjusted(i32 eval, i32 correction, i32 halfmove);

};