#pragma once

#include "data.h"

namespace order
{

enum class Stage
{
    HASHER,
    NOISY_GEN,
    NOISY,
    KILLER,
    QUIET_GEN,
    QUIET
};

class Picker
{
private:
    arrayvec<u16, move::MAX> moves;
    i32 scores[move::MAX];
    u16 hasher;
    u16 killer;
    usize index;
    Stage stage;
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

inline bool operator < (const Stage& a, const Stage& b)
{
    return static_cast<i32>(a) < static_cast<i32>(b);
};

};