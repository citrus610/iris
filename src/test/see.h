#pragma once

#include "../engine/see.h"

namespace test::static_exchange
{

struct Test
{
    std::string fen;
    u16 move;
    i32 threshold;
    bool result;
};

// Copied from pawnocchio
inline std::vector<Test> set = {
    Test {
        .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        .move = move::get<move::type::NORMAL>(square::A1, square::B2),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = true
    },
    Test {
        .fen = "k6b/8/8/8/8/2p5/1p6/BK6 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::A1, square::B2),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = false
    },
    Test {
        .fen = "k7/8/8/8/8/2p5/1p6/BK6 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::A1, square::B2),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = false
    },
    Test {
        .fen = "k7/8/8/8/8/2q5/1p6/BK6 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::A1, square::B2),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = true
    },
    Test {
        .fen = "k6b/8/8/8/8/2q5/1p6/BK6 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::A1, square::B2),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = false
    },
    Test {
        .fen = "k3n2r/3P4/8/8/8/8/8/1K6 w - - 0 1",
        .move = move::get<move::type::PROMOTION>(square::D7, square::E8, piece::type::QUEEN),
        .threshold = eval::PIECE_VALUE[piece::type::ROOK],
        .result = false
    },
    Test {
        .fen = "k3n3/3P4/8/8/8/8/8/1K6 w - - 0 1",
        .move = move::get<move::type::PROMOTION>(square::D7, square::E8, piece::type::QUEEN),
        .threshold = eval::PIECE_VALUE[piece::type::ROOK],
        .result = true
    },
    Test {
        .fen = "k3n3/3P4/8/8/8/8/8/1K6 w - - 0 1",
        .move = move::get<move::type::PROMOTION>(square::D7, square::E8, piece::type::QUEEN),
        .threshold = eval::PIECE_VALUE[piece::type::ROOK],
        .result = true
    },
    Test {
        .fen = "rn2k2r/p3bpp1/2p4p/8/2P3Q1/1P1q4/P4P1P/RNB1K2R w KQkq - 0 8",
        .move = move::get<move::type::NORMAL>(square::G4, square::G7),
        .threshold = 0,
        .result = true
    },
    Test {
        .fen = "r1bq1rk1/pppp1Npp/2nb1n2/4p3/2B1P3/2P5/PP1P1PPP/RNBQK2R b KQ - 0 6",
        .move = move::get<move::type::NORMAL>(square::F8, square::F7),
        .threshold = 0,
        .result = true
    },
    Test {
        .fen = "r1bqkb1r/ppp1pppp/2n2n2/8/2BPP3/5P2/PP4PP/RNBQK1NR b KQkq - 0 5",
        .move = move::get<move::type::NORMAL>(square::C6, square::D4),
        .threshold = 0,
        .result = true
    },
    Test {
        .fen = "3b2k1/1b6/8/3R2p1/4K3/5N2/8/8 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::F3, square::G5),
        .threshold = 0,
        .result = false
    },
    Test {
        .fen = "5k2/1b6/8/3B4/4K3/8/8/8 w - - 0 1",
        .move = move::get<move::type::NORMAL>(square::D5, square::B7),
        .threshold = 0,
        .result = true
    },
    Test {
        .fen = "6b1/k7/8/3Pp3/2K2N1r/8/8/8 w - e6 0 1",
        .move = move::get<move::type::ENPASSANT>(square::D5, square::E6),
        .threshold = 0,
        .result = true
    },
    Test {
        .fen = "6b1/k7/8/3Pp3/2K2N1r/8/8/8 w - e6 0 1",
        .move = move::get<move::type::ENPASSANT>(square::D5, square::E6),
        .threshold = 1,
        .result = false
    },
    Test {
        .fen = "6b1/k7/8/3Pp3/2K2N2/8/8/8 w - e6 0 1",
        .move = move::get<move::type::ENPASSANT>(square::D5, square::E6),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = true
    },
    Test {
        .fen = "k7/8/8/2KPp3/8/8/8/4r3 w - e6 0 1",
        .move = move::get<move::type::ENPASSANT>(square::D5, square::E6),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = false
    },
    Test {
        .fen = "k7/8/7R/2KPp3/8/8/8/4r3 w - e6 0 1",
        .move = move::get<move::type::ENPASSANT>(square::D5, square::E6),
        .threshold = eval::PIECE_VALUE[piece::type::PAWN],
        .result = true
    },
};

inline void test()
{
    std::cout << "SEE TEST" << std::endl;

    for (const auto& test : set) {
        auto board = Board(test.fen);
        auto see = see::is_ok(board, test.move, test.threshold);

        std::cout << std::endl;
        std::cout << test.fen << std::endl;
        std::cout << move::get_str(test.move) << std::endl;
        
        if (see == test.result) {
            std::cout << "passed!" << std::endl;
        }
        else {
            std::cout << "failed!" << std::endl;
        }
    }
};

};