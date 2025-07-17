#pragma once

#include "../engine/order.h"

namespace test::picker
{

struct Test
{
    std::string name;
    std::string fen;
    i32 depth;
};

inline std::vector<Test> set = {
    Test { .name = "startpos", .fen = Board::STARTPOS, .depth = 7 },
    Test { .name = "kiwipete", .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", .depth = 5 },
    Test { .name = "castling", .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", .depth = 6 },
    Test { .name = "enpassant", .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", .depth = 7 },
    Test { .name = "promotion", .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", .depth = 6 },
    Test { .name = "busy", .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", .depth = 5 }
};

inline bool check(Data& data, i32 depth)
{
    auto all = move::gen::get<move::gen::type::ALL>(data.board);

    for (auto mv : all) {
        if (data.board.is_quiet(mv)) {
            data.stack[data.ply].killer = mv;
            break;
        }
    }

    auto picker = order::Picker(data, all.size() ? all.back() : move::NONE);

    if (depth <= 1) {
        return true;
    }

    usize count = 0;

    while (true) {
        u16 move = picker.get(data);

        if (!move) {
            break;
        }

        count += 1;

        if (!data.board.is_legal(move)) {
            continue;
        }

        data.make(move);

        bool c = check(data, depth - 1);

        data.unmake(move);

        if (!c) {
            return false;
        }
    }

    if (count != all.size()) {
        data.board.print();
        std::cout << data.board.get_fen() << std::endl;

        return false;
    }

    return true;
};

inline void test()
{
    std::cout << "MOVE PICKER TEST" << std::endl;

    for (const auto& test : set) {
        auto data = Data(Board(test.fen));
        auto result = check(data, test.depth);

        std::cout << std::endl;
        std::cout << test.name << std::endl;
        
        if (result) {
            std::cout << "passed!" << std::endl;
        }
        else {
            std::cout << "failed!" << std::endl;
        }
    }
};

};