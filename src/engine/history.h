#pragma once

#include "eval.h"
#include "tune.h"

struct Data;

namespace history
{

inline i16 get_bonus(i32 depth)
{
    return std::min(depth * tune::HS_BONUS_COEF + tune::HS_BONUS_BIAS, i32(tune::HS_BONUS_MAX));
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

namespace history::corr
{

constexpr i32 MAX = 16384;
constexpr i32 SCALE = 8192;

constexpr usize SIZE = 1ULL << 14;
constexpr usize MASK = SIZE - 1;

inline i16 get_bonus(i32 delta, i32 depth)
{
    return std::clamp(delta * depth, -MAX / 4, MAX / 4);
};

class Table
{
private:
    i16 data[2][SIZE] = { 0 };
public:
    i16& get(const i8 color, const u64& hash);
    void update(const i8 color, const u64& hash, i16 bonus);
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
    history::corr::Table corr_pawn = {};
public:
    i32 get_score_quiet(Data& data, const u16& move);
    i32 get_score_noisy(Data& data, const u16& move);
    i32 get_correction(Board& board);
public:
    void update_correction(const i8 color, const u64& hash, i16 bonus);
};

};