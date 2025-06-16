#pragma once

#include "../chess/chess.h"

namespace eval::score
{

constexpr i32 DRAW = 0;
constexpr i32 MATE = 32000 + MAX_PLY;
constexpr i32 MATE_FOUND = MATE - MAX_PLY;
constexpr i32 NONE = MATE + 1;
constexpr i32 INFINITE = INT16_MAX;

constexpr i32 create(i32 midgame, i32 endgame)
{
    assert(midgame >= INT16_MIN && midgame <= INT16_MAX);
    assert(endgame >= INT16_MIN && endgame <= INT16_MAX);

    return midgame + static_cast<i32>(static_cast<u32>(endgame) << 16);
};

constexpr i32 get_midgame(i32 score)
{
    return static_cast<int16_t>(static_cast<uint16_t>(static_cast<u32>(score)));
};

constexpr i32 get_endgame(i32 score)
{
    return static_cast<int16_t>(static_cast<uint16_t>(static_cast<u32>(score + 0x8000) >> 16));
};

};

namespace eval
{

#define S(a, b) score::create(a, b)
#define SW(p, w, mg, eg) w.p = score::create(mg.p, eg.p)

// Piece values
constexpr i32 PIECE_VALUE[6] = {
    100, 320, 330, 500, 900, 100000
};

// Endgame scale
constexpr i32 SCALE_MAX = 128;

// Phase
const i32 PHASE_SCALE = 256;
const i32 PHASE_KNIGHT = 1;
const i32 PHASE_BISHOP = 1;
const i32 PHASE_ROOK = 2;
const i32 PHASE_QUEEN = 4;
const i32 PHASE_MAX = PHASE_KNIGHT * 4 + PHASE_BISHOP * 4 + PHASE_ROOK * 4 + PHASE_QUEEN * 2;

struct Weight
{
    i32 material[6];

    i32 table[2][6][64];

    i32 mobility_knight[9];
    i32 mobility_bishop[14];
    i32 mobility_rook[15];
    i32 mobility_queen[28];

    i32 king_defense[9];
    i32 king_open;
    i32 king_semiopen;

    i32 bishop_pair;

    i32 pawn_passed[8];
    i32 pawn_phalanx[8];
    
    i32 threat_pawn[3];
    i32 threat_minor[2];

    i32 tempo;
};

constexpr Weight MG = Weight {
    .material = {
        85, 320, 330, 500, 900, 10000
    },

    .table = {{ 0 },
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            98, 134, 61, 95, 68, 126, 34, -11,
            -6, 7, 26, 31, 65, 56, 25, -20,
            -14, 13, 6, 21, 23, 12, 17, -23,
            -27, -2, -5, 12, 17, 6, 10, -25,
            -26, -4, -4, -10, 3, 3, 33, -12,
            -35, -1, -20, -23, -15, 24, 38, -22,
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            -167, -89, -34, -49, 61, -97, -15, -107,
            -73, -41, 72, 36, 23, 62, 7, -17,
            -47, 60, 37, 65, 84, 129, 73, 44,
            -9, 17, 19, 53, 37, 69, 18, 22,
            -13, 4, 16, 13, 28, 19, 21, -8,
            -23, -9, 12, 10, 19, 17, 25, -16,
            -29, -53, -12, -3, -1, 18, -14, -19,
            -105, -21, -58, -33, -17, -28, -19, -23,
        },
        {
            -29, 4, -82, -37, -25, -42, 7, -8,
            -26, 16, -18, -13, 30, 59, 18, -47,
            -16, 37, 43, 40, 35, 50, 37, -2,
            -4, 5, 19, 50, 37, 37, 7, -2,
            -6, 13, 13, 26, 34, 12, 10, 4,
            0, 15, 15, 15, 14, 27, 18, 10,
            4, 15, 16, 0, 7, 21, 33, 1,
            -33, -3, -14, -21, -13, -12, -39, -21,
        },
        {
            32, 42, 32, 51, 63, 9, 31, 43,
            27, 32, 58, 62, 80, 67, 26, 44,
            -5, 19, 26, 36, 17, 45, 61, 16,
            -24, -11, 7, 26, 24, 35, -8, -20,
            -36, -26, -12, -1, 9, -7, 6, -23,
            -45, -25, -16, -17, 3, 0, -5, -33,
            -44, -16, -20, -9, -1, 11, -6, -71,
            -19, -13, 1, 17, 16, 7, -37, -26,
        },
        {
            -28, 0, 29, 12, 59, 44, 43, 45,
            -24, -39, -5, 1, -16, 57, 28, 54,
            -13, -17, 7, 8, 29, 56, 47, 57,
            -27, -27, -16, -16, -1, 17, -2, 1,
            -9, -26, -9, -10, -2, -4, 3, -3,
            -14, 2, -11, -2, -5, 2, 14, 5,
            -35, -8, 11, 2, 8, 15, -3, 1,
            -1, -18, -9, 10, -15, -25, -31, -50,
        },
        {
            -65, 23, 16, -15, -56, -34, 2, 13,
            29, -1, -20, -7, -8, -4, -38, -29,
            -9, 24, 2, -16, -20, 6, 22, -22,
            -17, -20, -12, -27, -30, -25, -14, -36,
            -49, -1, -27, -39, -46, -44, -33, -51,
            -14, -14, -22, -46, -44, -30, -15, -27,
            1, 7, -8, -64, -43, -16, 9, 8,
            -15, 36, 12, -54, 8, -28, 24, 14,
        }
    }},

    .mobility_knight = {
        -50, -25, -10, -5, 5, 10, 20, 30, 50
    },
    .mobility_bishop = {
        -50, -40, -15, -5, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
    },
    .mobility_rook = {
        -50, -40, -25, -15, -10, -10, -10, -5, 5, 10, 15, 20, 25, 40, 50,
    },
    .mobility_queen = {
        -50, -40, -25, -20, -15, -10, -5, 0, 5, 10, 10, 15, 15, 20, 20, 20, 25, 15, 15, 15, 25, 35, 35, 30, 10, 10, -20, -20
    },

    .king_defense = {
        -35, -15, 0, 15, 20, 25, 30, 15, 15
    },
    .king_open = -75,
    .king_semiopen = -30,

    .bishop_pair = 25,

    .pawn_passed = {
        0, 0, -5, -10, 10, 15, 25, 0
    },
    .pawn_phalanx = {
        0, 10, 20, 30, 50, 100, 150, 0
    },

    .threat_pawn = {
        50, 100, 50
    },
    .threat_minor = {
        50, 25
    }
};

constexpr Weight EG = Weight {
    .material = {
        100, 335, 350, 600, 1000, 10000
    },

    .table = {{ 0 },
    {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            178, 173, 158, 134, 147, 132, 165, 187,
            94, 100, 85, 67, 56, 53, 82, 84,
            32, 24, 13, 5, -2, 4, 17, 17,
            13, 9, -3, -7, -7, -8, 3, -1,
            4, 7, -6, 1, 0, -5, -1, -8,
            13, 8, 8, 10, 13, 0, 2, -7,
            0, 0, 0, 0, 0, 0, 0, 0,
        },
        {
            -58, -38, -13, -28, -31, -27, -63, -99,
            -25, -8, -25, -2, -9, -25, -24, -52,
            -24, -20, 10, 9, -1, -9, -19, -41,
            -17, 3, 22, 22, 22, 11, 8, -18,
            -18, -6, 16, 25, 16, 17, 4, -18,
            -23, -3, -1, 15, 10, -3, -20, -22,
            -42, -20, -10, -5, -2, -20, -23, -44,
            -29, -51, -23, -15, -22, -18, -50, -64,
        },
        {
            -14, -21, -11, -8, -7, -9, -17, -24,
            -8, -4, 7, -12, -3, -13, -4, -14,
            2, -8, 0, -1, -2, 6, 0, 4,
            -3, 9, 12, 9, 14, 10, 3, 2,
            -6, 3, 13, 19, 7, 10, -3, -9,
            -12, -3, 8, 10, 13, 3, -7, -15,
            -14, -18, -7, -1, 4, -9, -15, -27,
            -23, -9, -23, -5, -9, -16, -5, -17,
        },
        {
            13, 10, 18, 15, 12, 12, 8, 5,
            11, 13, 13, 11, -3, 3, 8, 3,
            7, 7, 7, 5, 4, -3, -5, -3,
            4, 3, 13, 1, 2, 1, -1, 2,
            3, 5, 8, 4, -5, -6, -8, -11,
            -4, 0, -5, -1, -7, -12, -8, -16,
            -6, -6, 0, 2, -9, -9, -11, -3,
            -9, 2, 3, -1, -5, -13, 4, -20,
        },
        {
            -9, 22, 22, 27, 27, 19, 10, 20,
            -17, 20, 32, 41, 58, 25, 30, 0,
            -20, 6, 9, 49, 47, 35, 19, 9,
            3, 22, 24, 45, 57, 40, 57, 36,
            -18, 28, 19, 47, 31, 34, 39, 23,
            -16, -27, 15, 6, 9, 17, 10, 5,
            -22, -23, -30, -16, -16, -23, -36, -32,
            -33, -28, -22, -43, -5, -32, -20, -41,
        },
        {
            -74, -35, -18, -18, -11, 15, 4, -17,
            -12, 17, 14, 17, 17, 38, 23, 11,
            10, 17, 23, 15, 20, 45, 44, 13,
            -8, 22, 24, 27, 26, 33, 26, 3,
            -18, -4, 21, 24, 27, 23, 9, -11,
            -19, -3, 11, 21, 23, 16, 7, -9,
            -27, -11, 4, 13, 14, 4, -5, -17,
            -53, -34, -21, -11, -28, -14, -24, -43
        }
    }},

    .mobility_knight = {
        -50, -25, -10, -5, 5, 10, 20, 30, 50
    },
    .mobility_bishop = {
        -50, -40, -15, -5, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
    },
    .mobility_rook = {
        -50, -40, -25, -15, -10, -10, -10, -5, 5, 10, 15, 20, 25, 40, 50,
    },
    .mobility_queen = {
        -50, -40, -25, -20, -15, -10, -5, 0, 5, 10, 10, 15, 15, 20, 20, 20, 25, 15, 15, 15, 25, 35, 35, 30, 10, 10, -20, -20
    },

    .king_defense = {
        -5, 0, 5, 5, 5, 0, -5, -10, -10
    },
    .king_open = 5,
    .king_semiopen = 15,

    .bishop_pair = 50,

    .pawn_passed = {
        0, 15, 20, 50, 75, 100, 150, 0
    },
    .pawn_phalanx = {
        0, 0, 15, 35, 100, 150, 250, 0
    },

    .threat_pawn = {
        20, -35, -15
    },
    .threat_minor = {
        0, 5
    }
};

constexpr Weight DEFAULT = [] {
    Weight w = Weight();

    for (i32 p = 0; p < 6; ++p) {
        SW(material[p], w, MG, EG);
    }

    for (i32 p = 0; p < 6; ++p) {
        for (i8 sq = 0; sq < 64; ++sq) {
            w.table[color::WHITE][p][sq] = score::create(MG.table[color::BLACK][p][sq ^ 56], EG.table[color::BLACK][p][sq ^ 56]);
            w.table[color::BLACK][p][sq] = score::create(MG.table[color::BLACK][p][sq], EG.table[color::BLACK][p][sq]);
        }
    }

    for (i32 i = 0; i < 9; ++i) {
        SW(mobility_knight[i], w, MG, EG);
    }

    for (i32 i = 0; i < 14; ++i) {
        SW(mobility_bishop[i], w, MG, EG);
    }

    for (i32 i = 0; i < 15; ++i) {
        SW(mobility_rook[i], w, MG, EG);
    }

    for (i32 i = 0; i < 28; ++i) {
        SW(mobility_queen[i], w, MG, EG);
    }

    for (i32 i = 0; i < 9; ++i) {
        SW(king_defense[i], w, MG, EG);
    }

    SW(king_open, w, MG, EG);
    SW(king_semiopen, w, MG, EG);

    SW(bishop_pair, w, MG, EG);

    for (i32 i = 0; i < 8; ++i) {
        SW(pawn_passed[i], w, MG, EG);
        SW(pawn_phalanx[i], w, MG, EG);
    }

    for (i32 i = 0; i < 3; ++i) {
        SW(threat_pawn[i], w, MG, EG);
    }

    for (i32 i = 0; i < 2; ++i) {
        SW(threat_minor[i], w, MG, EG);
    }

    w.tempo = 20;

    return w;
} ();

i32 get(Board& board);

i32 get_material(Board& board);

i32 get_table(Board& board);

i32 get_mobility(Board& board);

i32 get_king_defense(Board& board);

i32 get_bishop_pair(Board& board);

i32 get_pawn_structure(Board& board);

i32 get_threat(Board& board);

i32 get_open(Board& board);

i32 get_scale(Board& board, i32 eval);

i32 get_adjusted(i32 eval, i32 correction, i32 halfmove);

};