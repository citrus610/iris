#pragma once

#include "piece.h"
#include "square.h"
#include "castling.h"

namespace zobrist
{

void init();

u64 get_piece(i8 piece, i8 square);

u64 get_enpassant(i8 file);

u64 get_castling(i8 castling);

u64 get_color();

};