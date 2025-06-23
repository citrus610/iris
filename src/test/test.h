#pragma once

#include "perft.h"
#include "pseudo.h"
#include "gentype.h"
#include "quiet.h"
#include "picker.h"
#include "see.h"
#include "bench.h"
#include "nnue.h"

namespace test
{

inline void test()
{
    test::perft::test();
    test::pseudo::test();
    test::gentype::test();
    test::quiet::test();
    test::picker::test();
    test::static_exchange::test();
    test::bench::test();
    test::nn::test();
};

};