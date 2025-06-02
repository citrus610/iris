#pragma once

#include "data.h"
#include "see.h"

namespace order
{

enum class Stage
{
    HASHER,
    NOISY_GEN,
    NOISY_GOOD,
    KILLER,
    QUIET_GEN,
    QUIET,
    NOISY_BAD
};

class Picker
{
private:
    arrayvec<u16, move::MAX> moves;
    arrayvec<u16, move::MAX> baddies;
    i32 scores[move::MAX];
    u16 hasher;
    u16 killer;
    Stage stage;
    usize index;
    usize index_bad;
    bool skip;
public:
    Picker(Data& data, u16 hasher, bool skip = false);
public:
    u16 get(Data& data);
    Stage get_stage();
public:
    bool is_skipped();
public:
    void sort();
    void score_quiet(Data& data);
    void score_noisy(Data& data);
    void skip_quiets();
};

};

inline bool operator < (order::Stage a, order::Stage b)
{
    return static_cast<i32>(a) < static_cast<i32>(b);
};

inline bool operator > (order::Stage a, order::Stage b)
{
    return static_cast<i32>(a) > static_cast<i32>(b);
};