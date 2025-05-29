#include "pv.h"

namespace pv
{

Line::Line()
{
    this->clear();
};

void Line::clear()
{
    for (i32 i = 0; i < MAX_PLY + 8; ++i) {
        this->data[i] = move::NONE;
    }

    this->count = 0;
};

void Line::update(const u16& move, const Line& other)
{
    this->data[0] = move;

    for (i32 i = 0; i < other.count; ++i) {
        this->data[i + 1] = other.data[i];
    }

    this->count = other.count + 1;
};

};