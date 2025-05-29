#pragma once

#include "stack.h"

class Data
{
public:
    Board board;
    i32 ply;
    stack::Data stack;
    u64 nodes;
    i32 seldepth;
public:
    Data(const Board& board);
public:
    void clear();
public:
    void make(const u16& move);
    void unmake(const u16& move);
    void make_null();
    void unmake_null();
};