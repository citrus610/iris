#pragma once

#include "stack.h"
#include "pv.h"
#include "eval.h"
#include "table.h"
#include "timer.h"
#include "uci.h"

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