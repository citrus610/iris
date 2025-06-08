#pragma once

#include "attack.h"
#include "zobrist.h"

namespace cuckoo
{

alignas(64) inline u64 HASH[8192] = { 0 };
alignas(64) inline u16 MOVE[8192] = { 0 };

constexpr u64 get_h1(u64 hash)
{
    return hash & 0x1FFFULL;
};

constexpr u64 get_h2(u64 hash)
{
    return (hash >> 16) & 0x1FFFULL;
};

void init();

};