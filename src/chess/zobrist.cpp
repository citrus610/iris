#include "zobrist.h"

namespace zobrist
{

alignas(64) u64 PIECE[12][64];
alignas(64) u64 ENPASSANT[8];
alignas(64) u64 CASTLING[16];
alignas(64) u64 COLOR;

// Copied from Ethereal
u64 get_rand()
{
    static u64 seed = 1070372Ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717Ull;
};

void init()
{
    for (i8 piece = 0; piece < 12; ++piece) {
        for (i8 square = 0; square < 64; ++square) {
            zobrist::PIECE[piece][square] = zobrist::get_rand();
        }
    }

    for (i8 file = 0; file < 8; ++file) {
        zobrist::ENPASSANT[file] = zobrist::get_rand();
    }

    zobrist::CASTLING[0] = 0;
    zobrist::CASTLING[castling::WHITE_SHORT] = zobrist::get_rand();
    zobrist::CASTLING[castling::WHITE_LONG] = zobrist::get_rand();
    zobrist::CASTLING[castling::BLACK_SHORT] = zobrist::get_rand();
    zobrist::CASTLING[castling::BLACK_LONG] = zobrist::get_rand();

    for (i8 castling = 1; castling < 16; ++castling) {
        if ((castling & (castling - 1)) == 0) {
            continue;
        }

        u64 hash = 0;

        if (castling & castling::WHITE_SHORT) {
            hash ^= CASTLING[castling::WHITE_SHORT];
        }

        if (castling & castling::WHITE_LONG) {
            hash ^= CASTLING[castling::WHITE_LONG];
        }

        if (castling & castling::BLACK_SHORT) {
            hash ^= CASTLING[castling::BLACK_SHORT];
        }

        if (castling & castling::BLACK_LONG) {
            hash ^= CASTLING[castling::BLACK_LONG];
        }

        zobrist::CASTLING[castling] = hash;
    }

    zobrist::COLOR = zobrist::get_rand();
};

u64 get_piece(i8 piece, i8 square)
{
    assert(piece::is_valid(piece));
    assert(square::is_valid(square));

    return zobrist::PIECE[piece][square];
};

u64 get_enpassant(i8 file)
{
    assert(file::is_valid(file));

    return zobrist::ENPASSANT[file];
};

u64 get_castling(i8 castling)
{
    assert(castling::is_valid(castling));

    return zobrist::CASTLING[castling];
};

u64 get_color()
{
    return zobrist::COLOR;
};

};