#pragma once

#include "eval.h"
#include "stack.h"

namespace history
{

constexpr i32 BONUS_COEF = 128;
constexpr i32 BONUS_BIAS = -64;
constexpr i32 BONUS_MAX = 1024;

inline i16 get_bonus(i32 depth)
{
    return std::min(depth * BONUS_COEF + BONUS_BIAS, BONUS_MAX);
};

template <i16 MAX>
inline void update(i16& entry, i16 bonus)
{
    entry += bonus - entry * std::abs(bonus) / MAX;
};

};

namespace history::quiet
{

constexpr i32 MAX = 16384;

class Table
{
private:
    i16 data[2][64][64] = { 0 };
public:
    i16& get(Board& board, const u16& move);
    void update(Board& board, const u16& move, i16 bonus);
};

};

namespace history
{

struct Table
{
    history::quiet::Table quiet = {};
};

};