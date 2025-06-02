#pragma once

#include "eval.h"

struct Data;

namespace history
{

constexpr i32 BONUS_COEF = 150;
constexpr i32 BONUS_BIAS = -50;
constexpr i32 BONUS_MAX = 1000;

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

namespace history::noisy
{

constexpr i32 MAX = 16384;

class Table
{
private:
    i16 data[12][64][6] = { 0 };
public:
    i16& get(Board& board, const u16& move);
    i16& get(Board& board, const u16& move, i8 captured);
    void update(Board& board, const u16& move, i16 bonus);
};

};

namespace history::cont
{

constexpr i32 MAX = 16384;

class Entry
{
private:
    i16 data[12][64] = { 0 };
public:
    i16& get(Board& board, const u16& move);
    void update(Board& board, const u16& move, i16 bonus);
};

class Table
{
private:
    Entry data[12][64] = {};
public:
    Entry& get_entry(Board& board, const u16& move);
    i16 get(Data& data, const u16& move);
    i16 get(Data& data, const u16& move, i32 offset);
    void update(Data& data, const u16& move, i16 bonus);
    void update(Data& data, const u16& move, i32 offset, i16 bonus);
};

};

namespace history
{

class Table
{
public:
    history::quiet::Table quiet = {};
    history::noisy::Table noisy = {};
    history::cont::Table cont = {};
public:
    i32 get_score_quiet(Data& data, const u16& move);
    i32 get_score_noisy(Data& data, const u16& move);
};

};