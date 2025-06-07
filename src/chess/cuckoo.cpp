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
    for (auto i = 0; i < 8192; ++i) {
        TABLE[i] = 0ULL;
        A[i] = square::NONE;
        B[i] = square::NONE;
    }

    for (i8 piece = piece::WHITE_KNIGHT; piece < 12; ++piece) {
        for (i8 sq1 = 0; sq1 < 64; ++sq1) {
            for (i8 sq2 = sq1 + 1; sq2 < 64; ++sq2) {
                i8 a = sq1;
                i8 b = sq2;

                if (!is_reversible(piece::get_type(piece), a, b)) {
                    continue;
                }

                u64 hash = zobrist::get_piece(piece, a) ^ zobrist::get_piece(piece, b) ^ zobrist::get_color();
                u64 index = cuckoo::get_h1(hash);

                while (true)
                {
                    std::swap(TABLE[index], hash);
                    std::swap(A[index], a);
                    std::swap(B[index], b);

                    if (hash == 0ULL) {
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