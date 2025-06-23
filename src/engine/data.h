#pragma once

#include "history.h"
#include "stack.h"
#include "node.h"

class Data
{
public:
    u64 id;
public:
    Board board;
    i32 ply;
    stack::Data stack;
    nnue::Net nnue;
public:
    history::Table history {};
public:
    u64 nodes;
    i32 seldepth;
    node::Counter counter;
public:
    Data(const Board& board, u64 id = 0);
public:
    void clear();
public:
    void make(const u16& move);
    void unmake(const u16& move);
    void make_null();
    void unmake_null();
};