#pragma once

#include "attack.h"
#include "zobrist.h"

namespace cuckoo
{

alignas(64) inline u64 TABLE[8192];
alignas(64) inline i8 A[8192];
alignas(64) inline i8 B[8192];

constexpr u64 get_h1(u64 hash)
{
    return (hash >> 32) & 0x1FFFULL;
};

constexpr u64 get_h2(u64 hash)
{
    return (hash >> 48) & 0x1FFFULL;
};

void init();

};