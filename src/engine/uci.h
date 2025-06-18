#pragma once

#include "pv.h"
#include "eval.h"
#include "tune.h"

namespace uci
{

constexpr u64 HASH_DEFAULT = 8ULL;
constexpr u64 HASH_MIN = 1ULL;
constexpr u64 HASH_MAX = 1ULL << 16;

constexpr u64 THREAD_DEFAULT = 1ULL;
constexpr u64 THREAD_MIN = 1ULL;
constexpr u64 THREAD_MAX = 1ULL << 8;

};

namespace uci::parse
{

struct Setoption
{
    u64 hash = HASH_DEFAULT;
    u64 threads = THREAD_DEFAULT;
};

struct Go
{
    i32 depth;
    u64 time[2];
    u64 increment[2];
    std::optional<i32> movestogo;
    bool infinite;
};

std::optional<u16> move(const std::string& token, Board& board);

std::optional<Board> position(std::string in);

std::optional<Go> go(std::string in);

std::optional<Setoption> setoption(std::string in);

};

namespace uci::print
{

void option();

void info(i32 depth, i32 seldepth, i32 score, u64 nodes, u64 time, u64 hashfull, pv::Line pv);

void best(u16 move);

};