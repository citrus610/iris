#pragma once

#include "types.h"
#include "color.h"
#include "square.h"
#include "piece.h"
#include "move.h"
#include "bitboard.h"
#include "castling.h"
#include "attack.h"
#include "zobrist.h"
#include "board.h"
#include "movegen.h"

namespace chess
{

inline void init()
{
    zobrist::init();
    attack::init();
    bitboard::init();
};

};