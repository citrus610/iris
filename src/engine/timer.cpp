#include "timer.h"

namespace timer
{

Data::Data()
{
    this->clear();
};

void Data::set(uci::parse::Go go, i8 color)
{
    this->start = timer::get_current();
    this->limit_soft = this->start + timer::get_available_soft(go.time[color], go.increment[color], go.movestogo);
    this->limit_hard = this->start + timer::get_available_hard(go.time[color]);

    if (go.infinite) {
        this->limit_soft = UINT64_MAX;
        this->limit_hard = UINT64_MAX;
    }
};

void Data::clear()
{
    this->start = 0;
    this->limit_soft = UINT64_MAX;
    this->limit_hard = UINT64_MAX;
};

bool Data::is_over_soft(f64 nodes_ratio, i32 pv_stability)
{
    f64 remain = this->limit_soft - this->start;

    remain *= 2.0 - 1.5 * nodes_ratio;
    remain *= 1.25 - 0.05 * f64(pv_stability);

    return timer::get_current() >= this->start + u64(remain);
};

bool Data::is_over_hard()
{
    return timer::get_current() >= this->limit_hard;
};

};