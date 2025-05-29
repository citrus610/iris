#pragma once

#include "../chess/chess.h"

namespace pv
{

class Line
{
public:
    u16 data[MAX_PLY + 8] = { move::NONE };
    i32 count = 0;
public:
    Line();
public:
    void clear();
    void update(const u16& move, const Line& other);
};

};