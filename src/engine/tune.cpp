#include "tune.h"

namespace tune
{

namespace lmr
{
    i32 TABLE[MAX_PLY][move::MAX];
};

Value::Value(std::string name, i32 value, i32 min, i32 max, i32 step, bool tunable)
{
    this->name = name;
    this->value = value;
    this->min = min;
    this->max = max;
    this->step = step;

    if (tunable) {
        values.push_back(this);
    }
};

void init()
{
    const double coef = double(LMR_COEF) / 100.0;
    const double bias = double(LMR_BIAS) / 100.0;

    // Late move reduction table
    for (i32 i = 0; i < MAX_PLY; ++i) {
        for (usize k = 0; k < move::MAX; ++k) {
            if (i == 0 || k == 0) {
                lmr::TABLE[i][k] = 0;
                continue;
            }

            lmr::TABLE[i][k] = i32(std::log(i) * std::log(k) * coef + bias);
        }
    }
};

Value* find(std::string name)
{
    for (auto value : values) {
        if (value->name == name) {
            return value;
        }
    }

    return nullptr;
};

};