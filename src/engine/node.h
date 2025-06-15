#pragma once

#include "eval.h"

namespace node
{

enum class Type
{
    ROOT,
    PV,
    NORMAL
};

class Counter
{
public:
    u64 data[64 * 64] = { 0 };
public:
    u64 get(const u16& move);
    void set(const u16& move, u64 count);
};

};