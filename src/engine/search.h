#pragma once

#include "order.h"
#include "table.h"
#include "timer.h"
#include "uci.h"
#include "tune.h"
#include "see.h"

namespace search
{

enum class Node
{
    ROOT,
    PV,
    NORMAL
};

class Engine
{
private:
    std::atomic_flag running;
    std::thread* thread;
private:
    timer::Data timer;
    transposition::Table table;
public:
    u64 nodes;
    u64 time;
public:
    Engine();
public:
    void clear();
    void set(uci::parse::Setoption uci_setoption);
    bool stop();
    bool join();
    template <bool BENCH> bool search(Board uci_board, uci::parse::Go uci_go);
public:
    i32 aspiration_window(Data& data, i32 depth, i32 score_old);
    template <Node NODE> i32 pvsearch(Data& data, i32 alpha, i32 beta, i32 depth);
    template <bool PV> i32 qsearch(Data& data, i32 alpha, i32 beta);
};

void init();

};