#include "node.h"

namespace node
{

u64 Counter::get(const u16& move)
{
    return this->data[move & 0xFFF];
};

void Counter::set(const u16& move, u64 count)
{
    this->data[move & 0xFFF] = count;
};

};