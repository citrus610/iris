#pragma once

#include "../chess/chess.h"

namespace tune
{

namespace aw
{
    constexpr i32 DEPTH = 4;
    constexpr i32 DELTA = 25;
};

namespace rfp
{
    constexpr i32 DEPTH = 8;
    constexpr i32 COEF = 50;
};

namespace nmp
{
    constexpr i32 DEPTH = 3;
    constexpr i32 REDUCTION = 4;
    constexpr i32 REDUCTION_EVAL_MAX = 3;
    constexpr i32 DIVISOR_DEPTH = 5;
    constexpr i32 DIVISOR_EVAL = 200;
};

};