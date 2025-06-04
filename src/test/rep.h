#pragma once

#include "../chess/chess.h"

namespace test::rep
{

void test()
{
    std::cout << "IS MOVE QUIET TEST" << std::endl;

    auto board = Board();

    std::vector<u16> moves = {
        move::get<move::type::NORMAL>(square::G1, square::F3),
        move::get<move::type::NORMAL>(square::G8, square::F6),
        move::get<move::type::NORMAL>(square::F3, square::G1),
        move::get<move::type::NORMAL>(square::F6, square::G8),
        move::get<move::type::NORMAL>(square::G1, square::F3),
        move::get<move::type::NORMAL>(square::G8, square::F6),
        move::get<move::type::NORMAL>(square::F3, square::G1),
        move::get<move::type::NORMAL>(square::F6, square::G8),
        move::get<move::type::NORMAL>(square::G1, square::F3),
        move::get<move::type::NORMAL>(square::G8, square::F6),
        move::get<move::type::NORMAL>(square::F3, square::G1),
        move::get<move::type::NORMAL>(square::F6, square::G8)
    };

    for ()
};

};