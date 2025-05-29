#include "timer.h"

namespace timer
{

Data::Data()
{
    this->clear();
};

void Data::set(uci::parse::Go go, i8 color)
{
    const u64 now = timer::get_current();

    this->limit_soft = now + timer::get_available_soft(go.time[color], go.increment[color], go.movestogo);
    this->limit_hard = now + timer::get_available_hard(go.time[color]);

    if (go.infinite) {
        this->clear();
    }
};

void Data::clear()
{
    this->limit_soft = UINT64_MAX;
    this->limit_hard = UINT64_MAX;
};

};