#pragma once

#include "pv.h"
#include "eval.h"
#include "history.h"

namespace stack
{

struct Entry
{
    pv::Line pv = pv::Line();
    u16 move = move::NONE;
    u16 killer = move::NONE;
    u16 excluded = move::NONE;
    i32 eval = eval::score::NONE;
    history::cont::Entry* conthist = nullptr;
};

class Data
{
private:
    Entry data[MAX_PLY + 8];
public:
    Data();
public:
    void clear();
public:
    Entry& operator [] (usize index);
    const Entry& operator [] (usize index) const;
};

inline Entry& Data::operator [] (usize index)
{
    assert(index < MAX_PLY + 8);
    return this->data[index];
};

inline const Entry& Data::operator [] (usize index) const
{
    assert(index < MAX_PLY + 8);
    return this->data[index];
};

};