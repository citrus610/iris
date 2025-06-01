#include "tune.h"

namespace tune
{

namespace lmr
{
    i32 TABLE[MAX_PLY][move::MAX];
};

void init()
{
    // Late move reduction table
    for (i32 i = 0; i < MAX_PLY; ++i) {
        for (usize k = 0; k < move::MAX; ++k) {
            if (i == 0 || k == 0) {
                lmr::TABLE[i][k] = 0;
                continue;
            }

            lmr::TABLE[i][k] = i32(std::log(i) * std::log(k) * lmr::COEF + lmr::BIAS);
        }
    }
};

};