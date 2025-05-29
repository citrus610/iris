#pragma once

#include "../chess/chess.h"

namespace test::perft
{

template <bool ROOT>
inline u64 get(Board& board, i32 depth)
{
    auto moves = move::gen::get<move::gen::type::ALL>(board);

    u64 count = 0;

    if (depth <= 1) {
        for (const u16& move : moves) {
            if (board.is_legal(move)) {
                count += 1;

                if constexpr (ROOT) {
                    std::cout << move::get_str(move) << " - " << 1 << std::endl;
                }
            }
        }

        return count;
    }

    for (const u16& move : moves) {
        if (!board.is_legal(move)) {
            continue;
        }

        board.make(move);

        u64 nodes = perft::get<false>(board, depth - 1);

        board.unmake(move);

        if constexpr (ROOT) {
            std::cout << move::get_str(move) << " - " << nodes << std::endl;
        }

        count += nodes;
    }

    return count;
};

struct Test
{
    std::string name;
    std::string fen;
    i32 depth;
    u64 count;
};

inline std::vector<Test> set = {
    Test { .name = "startpos", .fen = Board::STARTPOS, .depth = 7, .count = 3195901860 },
    Test { .name = "kiwipete", .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", .depth = 5, .count = 193690690 },
    Test { .name = "castling", .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", .depth = 6, .count = 706045033 },
    Test { .name = "enpassant", .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", .depth = 7, .count = 178633661 },
    Test { .name = "promotion", .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", .depth = 6, .count = 3048196529 },
    Test { .name = "busy", .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", .depth = 5, .count = 164075551 }
};

inline void test()
{
    std::cout << "PERFT TEST" << std::endl;

    for (const auto& data : set) {
        auto board = Board(data.fen);

        auto t1 = std::chrono::high_resolution_clock::now();
        auto count = perft::get<false>(board, data.depth);
        auto t2 = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << std::endl;
        std::cout << data.name << std::endl;
        std::cout << " - depth: " << data.depth << std::endl;
        std::cout << " - count: " << count << std::endl;
        std::cout << " - time: " << time << " ms" << std::endl;
        std::cout << " - nps: " << (count / time) << " kn/s" << std::endl;
        
        if (count == data.count) {
            std::cout << "passed!" << std::endl;
        }
        else {
            std::cout << "failed!" << std::endl;
        }
    }
};

};