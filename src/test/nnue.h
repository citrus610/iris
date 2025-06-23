#pragma once

#include "../engine/search.h"

namespace test::nn
{

inline bool check(Board& board, nnue::Net& nnue, i32 depth)
{
    auto moves = move::gen::get<move::gen::type::ALL>(board);

    if (depth <= 0) {
        return true;
    }

    for (const u16& move : moves) {
        if (!board.is_legal(move)) {
            continue;
        }

        nnue.make(board, move);
        board.make(move);

        auto raw = nnue::Net();
        raw.refresh(board);

        if (nnue.get_eval(board.get_color()) != raw.get_eval(board.get_color())) {
            nnue.unmake();
            board.unmake(move);

            std::cout << "ERROR: \n";
            board.print();
            std::cout << "move: " << move::get_str(move) << "\n";

            return false;
        }

        bool c = check(board, nnue, depth - 1);

        nnue.unmake();
        board.unmake(move);

        if (!c) {
            return false;
        }
    }

    return true;
};

struct Test
{
    std::string name;
    std::string fen;
    i32 depth;
};

inline std::vector<Test> set = {
    Test { .name = "startpos", .fen = Board::STARTPOS, .depth = 5 },
    Test { .name = "kiwipete", .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", .depth = 5 },
    Test { .name = "castling", .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", .depth = 5 },
    Test { .name = "enpassant", .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", .depth = 5 },
    Test { .name = "promotion", .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", .depth = 5 },
    Test { .name = "busy", .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", .depth = 4 }
};

inline void test()
{
    std::cout << "MOVE GEN TYPE TEST" << std::endl;

    for (const auto& test : set) {
        auto nnue = nnue::Net();
        auto board = Board(test.fen);

        nnue.refresh(board);

        auto result = check(board, nnue, test.depth);

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