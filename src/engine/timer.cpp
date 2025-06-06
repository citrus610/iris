#include "timer.h"

namespace timer
{

Data::Data()
{
    this->clear();
};

void Data::set(uci::parse::Go go, i8 color)
{
    this->clear();

    this->now = timer::get_current();

    if (go.infinite) {
        return;
    }

    this->limit_soft = this->now + timer::get_available_soft(go.time[color], go.increment[color], go.movestogo);
    this->limit_hard = this->now + timer::get_available_hard(go.time[color]);
};

void Data::clear()
{
    this->now = 0;
    this->limit_soft = UINT64_MAX;
    this->limit_hard = UINT64_MAX;
};

bool Data::is_over_soft(i32 stability_eval, i32 stability_pv)
{
    double soft = this->limit_soft - this->now;

    soft *= 1.25 - 0.05 * double(stability_eval);
    soft *= 1.25 - 0.05 * double(stability_pv);

    return timer::get_current() >= this->now + u64(std::round(soft));
};

bool Data::is_over_hard()
{
    return timer::get_current() >= this->limit_hard;
};

};