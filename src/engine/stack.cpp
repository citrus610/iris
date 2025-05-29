#include "stack.h"

namespace stack
{

Data::Data()
{
    this->clear();
};

void Data::clear()
{
    for (usize i = 0; i < MAX_PLY + 8; ++i) {
        this->data[i] = Entry();
    }
};

};