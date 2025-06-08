#include "cuckoo.h"

namespace cuckoo
{

bool is_reversible(i8 type, i8 a, i8 b)
{
    assert(piece::type::is_valid(type));
    assert(type != piece::type::PAWN);

    u64 attacks = 0ULL;

    switch (type)
    {
    case piece::type::KNIGHT:
        attacks = attack::get_knight(a);
        break;
    case piece::type::BISHOP:
        attacks = attack::get_bishop(a, 0ULL);
        break;
    case piece::type::ROOK:
        attacks = attack::get_rook(a, 0ULL);
        break;
    case piece::type::QUEEN:
        attacks = attack::get_queen(a, 0ULL);
        break;
    case piece::type::KING:
        attacks = attack::get_king(a);
        break;
    }

    return attacks & bitboard::create(b);
};

void init()
{
    for (i8 piece = piece::WHITE_KNIGHT; piece < 12; ++piece) {
        for (i8 a = 0; a < 64; ++a) {
            for (i8 b = a + 1; b < 64; ++b) {
                if (!is_reversible(piece::get_type(piece), a, b)) {
                    continue;
                }

                u16 move = move::get<move::type::NORMAL>(a, b);
                u64 hash = zobrist::get_piece(piece, a) ^ zobrist::get_piece(piece, b) ^ zobrist::get_color();
                u64 index = cuckoo::get_h1(hash);

                while (true)
                {
                    std::swap(cuckoo::HASH[index], hash);
                    std::swap(cuckoo::MOVE[index], move);

                    if (move == move::NONE) {
                        break;
                    }

                    if (index == cuckoo::get_h1(hash)) {
                        index = cuckoo::get_h2(hash);
                    }
                    else {
                        index = cuckoo::get_h1(hash);
                    }
                }
            }
        }
    }
};

};