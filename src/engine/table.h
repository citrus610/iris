#pragma once

#include <thread>
#include <climits>
#include <cstring>

#include "../util/alloc.h"
#include "eval.h"

namespace transposition
{

namespace bound
{

constexpr u8 NONE = 0;
constexpr u8 UPPER = 1;
constexpr u8 LOWER = 2;
constexpr u8 EXACT = 3;

};

namespace mask
{

constexpr u8 AGE = 0b11111000;
constexpr u8 PV = 0b100;
constexpr u8 BOUND = 0b11;

};

constexpr usize MAX_ENTRIES = 3;
constexpr u8 MAX_AGE = 1 << std::popcount(mask::AGE);

constexpr u64 KB = 1ULL << 10;
constexpr u64 MB = 1ULL << 20;

class Entry
{
private:
    u16 hash = 0;
    u16 move = move::NONE;
    i16 score = 0;
    i16 eval = 0;
    u8 depth = 0;
    u8 flags = 0; // age : 5, pv : 1, bound : 2
public:
    u16 get_hash();
    u16 get_move();
    i32 get_score(i32 ply);
    i32 get_eval();
    i32 get_depth();
    u8 get_age();
    u8 get_bound();
    i32 get_age_distance(u8 age);
    bool is_pv();
public:
    void set_score(i32 score, i32 ply);
    void set(u64 hash, u16 move, i32 score, i32 eval, i32 depth, u8 age, bool pv, u8 bound, i32 ply);
};

struct alignas(32) Bucket
{
    Entry entries[MAX_ENTRIES];
};

class Table
{
public:
    Bucket* buckets;
    u64 count;
    u8 age;
public:
    Table();
    ~Table();
public:
    void init(u64 mb);
    void clear(usize thread_count = 1);
public:
    std::pair<bool, Entry*> get(u64 hash);
    u64 get_index(u64 hash);
public:
    void update();
    void prefetch(u64 hash);
    usize hashfull();
};

static_assert(sizeof(Entry) == 10);
static_assert(sizeof(Bucket) == 32);

};