#include "timer.h"

namespace timer
{

Data::Data()
{
    this->clear();
};

void Data::set(uci::parse::Go go, i8 color)
{
    this->now = timer::get_current();
    this->limit_soft = this->now + timer::get_available_soft(go.time[color], go.increment[color], go.movestogo);
    this->limit_hard = this->now + timer::get_available_hard(go.time[color]);

    if (go.infinite) {
        this->limit_soft = UINT64_MAX;
        this->limit_hard = UINT64_MAX;
    }
};

void Data::clear()
{
    this->now = 0;
    this->limit_soft = UINT64_MAX;
    this->limit_hard = UINT64_MAX;
};

bool Data::is_over_soft()
{
    return timer::get_current() >= this->limit_soft;
};

bool Data::is_over_hard()
{
    return timer::get_current() >= this->limit_hard;
};

};