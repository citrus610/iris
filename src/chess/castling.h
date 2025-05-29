#pragma once

#include "square.h"

namespace castling
{

constexpr i8 NONE = 0;

constexpr i8 WHITE_SHORT = 1;
constexpr i8 WHITE_LONG = 1 << 1;
constexpr i8 BLACK_SHORT = 1 << 2;
constexpr i8 BLACK_LONG = 1 << 3;

constexpr i8 WHITE = WHITE_SHORT | WHITE_LONG;
constexpr i8 BLACK = BLACK_SHORT | BLACK_LONG;

constexpr i8 SHORT = WHITE_SHORT | BLACK_SHORT;
constexpr i8 LONG = WHITE_LONG | BLACK_LONG;

constexpr i8 ALL = WHITE | BLACK;

constexpr bool is_valid(i8 castling_right)
{
    return castling_right > 0 && castling_right < 16;
};

constexpr i8 create(i8 corner)
{
    assert(square::is_valid(corner));

    switch (corner)
    {
    case square::A1:
        return castling::WHITE_LONG;
    case square::H1:
        return castling::WHITE_SHORT;
    case square::A8:
        return castling::BLACK_LONG;
    case square::H8:
        return castling::BLACK_SHORT;
    }

    return castling::NONE;
};

constexpr i8 get_king_to(i8 color, bool castle_short)
{
    assert(color::is_valid(color));

    return square::get_relative(castle_short ? square::G1 : square::C1, color);
};

constexpr i8 get_rook_to(i8 color, bool castle_short)
{
    assert(color::is_valid(color));

    return square::get_relative(castle_short ? square::F1 : square::D1, color);
};

constexpr i8 get_rook_from(i8 color, bool castle_short)
{
    assert(color::is_valid(color));

    return square::get_relative(castle_short ? square::H1 : square::A1, color);
};

};