#pragma once

#include "uci.h"

namespace timer
{

class Data
{
public:
    u64 limit_soft;
    u64 limit_hard;
public:
    Data();
public:
    void set(uci::parse::Go go, i8 color);
    void clear();
};

inline u64 get_current()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
};

inline u64 get_available_soft(u64 remain, u64 increment, std::optional<u64> movestogo = {})
{
    u64 mtg = movestogo.value_or(45) + 5;

    return (remain - increment) / mtg + increment / 2;
};

inline u64 get_available_hard(u64 remain)
{
    return remain / 2;
};

};