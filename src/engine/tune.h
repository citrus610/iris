#pragma once

#include "../chess/chess.h"

namespace tune
{

class Value
{
public:
    std::string name;
    i32 value;
    i32 min;
    i32 max;
    i32 step;
public:
    Value(std::string name, i32 value, i32 min, i32 max, i32 step, bool tunable);
public:
    inline operator i32 () { return this->value; };
};

inline std::vector<Value*> values;

// #define TUNE 1

#ifdef TUNE
    constexpr bool TUNING = true;
    #define VALUE(name, value, min, max, step, tunable) inline auto name = Value(#name, value, min, max, step, tunable);
#else
    constexpr bool TUNING = false;
    #define VALUE(name, value, min, max, step, tunable) constexpr i32 name = value;
#endif

VALUE(AW_DEPTH, 4, 3, 6, 1, false)
VALUE(AW_DELTA, 25, 10, 50, 5, false)

VALUE(RAZOR_COEF, 300, 100, 500, 20, true)

VALUE(RFP_DEPTH, 8, 6, 12, 1, false)
VALUE(RFP_COEF, 63, 20, 100, 10, true)

VALUE(NMP_DEPTH, 3, 3, 8, 1, false)
VALUE(NMP_REDUCTION, 4, 2, 5, 1, false)
VALUE(NMP_REDUCTION_EVAL_MAX, 3, 1, 5, 1, false)
VALUE(NMP_DIVISOR_DEPTH, 5, 2, 8, 1, false)
VALUE(NMP_DIVISOR_EVAL, 188, 50, 500, 25, true)

VALUE(LMP_BASE, 3, 3, 3, 0, false)

VALUE(FP_COEF, 78, 50, 500, 10, true)
VALUE(FP_BIAS, 60, 0, 250, 10, true)
VALUE(FP_DEPTH, 10, 8, 12, 1, false)
VALUE(FP_MARGIN_QS, 128, 20, 500, 10, true)

VALUE(SEEP_MARGIN_QS, -16, -250, 0, 10, true)
VALUE(SEEP_MARGIN_QUIET, -80, -250, -50, 10, true)
VALUE(SEEP_MARGIN_NOISY, -17, -50, -10, 10, true)

VALUE(LMR_DEPTH, 3, 0, 0, 1, false)
VALUE(LMR_COEF, 40, 25, 50, 5, false)
VALUE(LMR_BIAS, 80, 50, 100, 5, false)
VALUE(LMR_HIST_DIV, 8192, 1024, 16384, 512, true)
VALUE(LMR_MORE_COEF, 40, 20, 100, 5, true)

VALUE(IIR_DEPTH, 4, 0, 0, 10, false)

VALUE(SE_DEPTH, 8, 4, 12, 1, false)
VALUE(SE_DOUBLE_BIAS, 16, 4, 20, 1, false)
VALUE(SE_TRIPLE_BIAS, 100, 50, 500, 20, false)

VALUE(HS_BONUS_COEF, 178, 50, 500, 20, false)
VALUE(HS_BONUS_BIAS, -50, -250, 0, 20, false)
VALUE(HS_BONUS_MAX, 1500, 500, 2500, 100, false)

VALUE(HS_MALUS_COEF, 150, 50, 500, 20, false)
VALUE(HS_MALUS_BIAS, -50, -250, 0, 20, false)
VALUE(HS_MALUS_MAX, 1000, 500, 2500, 100, false)

VALUE(CORR_WEIGHT_PAWN, 64, 16, 128, 16, false)
VALUE(CORR_WEIGHT_NON_PAWN, 32, 16, 128, 16, false)

inline i32 LMR_TABLE[MAX_PLY][move::MAX];

void init();

Value* find(std::string name);

void print_spsa();

};