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
    u16& operator [] (usize index);
    const u16& operator [] (usize index) const;
public:
    void clear();
    void update(const u16& move, const Line& other);
};

inline u16& Line::operator [] (usize index)
{
    return this->data[index];
};

inline const u16& Line::operator [] (usize index) const
{
    return this->data[index];
};

};