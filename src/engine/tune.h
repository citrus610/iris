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

namespace lmp
{
    constexpr i32 BASE = 3;
};

namespace fp
{
    constexpr i32 COEF = 100;
    constexpr i32 BIAS = 50;
    constexpr i32 DEPTH = 10;
    constexpr i32 MARGIN_QS = 150;
};

namespace seep
{
    constexpr i32 MARGIN_QS = -50;
};

namespace lmr
{
    constexpr i32 DEPTH = 3;
    constexpr double COEF = 0.35;
    constexpr double BIAS = 0.8;

    extern i32 TABLE[MAX_PLY][move::MAX];
};

namespace iir
{
    constexpr i32 DEPTH = 3;
};

void init();

};