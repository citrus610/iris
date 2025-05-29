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
    i32 material_pawn;
    i32 material_knight;
    i32 material_bishop;
    i32 material_rook;
    i32 material_queen;
    i32 material_king;

    i32 table[6][64];

    i32 mobility_knight[9];
    i32 mobility_bishop[14];
    i32 mobility_rook[15];
    i32 mobility_queen[28];

    i32 king_defense[9];

    i32 bishop_pair;

    i32 pawn_passed[8];

    i32 tempo;
};

constexpr Weight MG = Weight {
    .material_pawn = 100,
    .material_knight = 320,
    .material_bishop = 330,
    .material_rook = 500,
    .material_queen = 900,
    .material_king = 10000,

    .table = {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            5, 10, 10, -20, -20, 10, 10, 5,
            5, -5, -10, 0, 0, -10, -5, 5,
            0, 0, 0, 20, 20, 0, 0, 0,
            5, 5, 10, 25, 25, 10, 5, 5,
            10, 10, 20, 30, 30, 20, 10, 10,
            50, 50, 50, 50, 50, 50, 50, 50,
            0, 0, 0, 0, 0, 0, 0, 0
        },
        {
            -50, -40, -30, -30, -30, -30, -40, -50,
            -40, -20, 0, 5, 5, 0, -20, -40,
            -30, 5, 10, 15, 15, 10, 5, -30,
            -30, 0, 15, 20, 20, 15, 0, -30,
            -30, 5, 15, 20, 20, 15, 5, -30,
            -30, 0, 10, 15, 15, 10, 0, -30,
            -40, -20, 0, 0, 0, 0, -20, -40,
            -50, -40, -30, -30, -30, -30, -40, -50
        },
        {
            -20, -10, -10, -10, -10, -10, -10, -20,
            -10, 5, 0, 0, 0, 0, 5, -10,
            -10, 10, 10, 10, 10, 10, 10, -10,
            -10, 0, 10, 10, 10, 10, 0, -10,
            -10, 5, 5, 10, 10, 5, 5, -10,
            -10, 0, 5, 10, 10, 5, 0, -10,
            -10, 0, 0, 0, 0, 0, 0, -10,
            -20, -10, -10, -10, -10, -10, -10, -20
        },
        {
            0, 0, 0, 5, 5, 0, 0, 0,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            5, 10, 10, 10, 10, 10, 10, 5,
            0, 0, 0, 0, 0, 0, 0, 0
        },
        {
            -20, -10, -10, -5, -5, -10, -10, -20,
            -10, 0, 0, 0, 0, 5, 0, -10,
            -10, 0, 5, 5, 5, 5, 5, -10,
            0, 0, 5, 5, 5, 5, 0, -5,
            -5, 0, 5, 5, 5, 5, 0, -5,
            -10, 0, 5, 5, 5, 5, 0, -10,
            -10, 0, 0, 0, 0, 0, 0, -10,
            -20, -10, -10, -5, -5, -10, -10, -20
        },
        {
            20, 30, 10, 0, 0, 10, 30, 20,
            20, 20, 0, 0, 0, 0, 20, 20,
            -10, -20, -20, -20, -20, -20, -20, -10,
            -20, -30, -30, -40, -40, -30, -30, -20,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30,
            -30, -40, -40, -50, -50, -40, -40, -30,
        }
    },

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

    .bishop_pair = 25,

    .pawn_passed = {
        0, 0, -5, -10, 10, 15, 25, 0
    }
};

constexpr Weight EG = Weight {
    .material_pawn = 100,
    .material_knight = 320,
    .material_bishop = 330,
    .material_rook = 500,
    .material_queen = 1000,
    .material_king = 10000,

    .table = {
        {
            0, 0, 0, 0, 0, 0, 0, 0,
            5, 5, 5, -20, -20, 5, 5, 5,
            10, 10, 10, 10, 10, 10, 10, 10,
            20, 20, 20, 20, 20, 20, 20, 20,
            30, 30, 30, 30, 30, 30, 30, 30,
            40, 40, 40, 40, 40, 40, 40, 40,
            50, 50, 50, 50, 50, 50, 50, 50,
            0, 0, 0, 0, 0, 0, 0, 0
        },
        {
            -50, -40, -30, -30, -30, -30, -40, -50,
            -40, -20, 0, 5, 5, 0, -20, -40,
            -30, 5, 10, 15, 15, 10, 5, -30,
            -30, 0, 15, 20, 20, 15, 0, -30,
            -30, 5, 15, 20, 20, 15, 5, -30,
            -30, 0, 10, 15, 15, 10, 0, -30,
            -40, -20, 0, 0, 0, 0, -20, -40,
            -50, -40, -30, -30, -30, -30, -40, -50
        },
        {
            -20, -10, -10, -10, -10, -10, -10, -20,
            -10, 5, 0, 0, 0, 0, 5, -10,
            -10, 10, 10, 10, 10, 10, 10, -10,
            -10, 0, 10, 10, 10, 10, 0, -10,
            -10, 5, 5, 10, 10, 5, 5, -10,
            -10, 0, 5, 10, 10, 5, 0, -10,
            -10, 0, 0, 0, 0, 0, 0, -10,
            -20, -10, -10, -10, -10, -10, -10, -20
        },
        {
            0, 0, 0, 5, 5, 0, 0, 0,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            -5, 0, 0, 0, 0, 0, 0, -5,
            5, 10, 10, 10, 10, 10, 10, 5,
            0, 0, 0, 0, 0, 0, 0, 0
        },
        {
            -20, -10, -10, -5, -5, -10, -10, -20,
            -10, 0, 0, 0, 0, 5, 0, -10,
            -10, 0, 5, 5, 5, 5, 5, -10,
            0, 0, 5, 5, 5, 5, 0, -5,
            -5, 0, 5, 5, 5, 5, 0, -5,
            -10, 0, 5, 5, 5, 5, 0, -10,
            -10, 0, 0, 0, 0, 0, 0, -10,
            -20, -10, -10, -5, -5, -10, -10, -20
        },
        {
            -50, -30, -30, -30, -30, -30, -30, -50,
            -30, -30, 0, 0, 0, 0, -30, -30,
            -30, -10, 20, 30, 30, 20, -10, -30,
            -30, -10, 30, 40, 40, 30, -10, -30,
            -30, -10, 30, 40, 40, 30, -10, -30,
            -30, -10, 20, 30, 30, 20, -10, -30,
            -30, -20, -10, 0, 0, -10, -20, -30,
            -50, -40, -30, -20, -20, -30, -40, -50,
        }
    },

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

    .bishop_pair = 50,

    .pawn_passed = {
        0, 15, 20, 50, 75, 100, 150, 0
    }
};

constexpr Weight DEFAULT = [] {
    Weight w = Weight();

    SW(material_pawn, w, MG, EG);
    SW(material_knight, w, MG, EG);
    SW(material_bishop, w, MG, EG);
    SW(material_rook, w, MG, EG);
    SW(material_queen, w, MG, EG);
    SW(material_king, w, MG, EG);

    for (i32 p = 0; p < 6; ++p) {
        for (i32 sq = 0; sq < 64; ++sq) {
            SW(table[p][sq], w, MG, EG);
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

    SW(bishop_pair, w, MG, EG);

    for (i32 i = 0; i < 8; ++i) {
        SW(pawn_passed[i], w, MG, EG);
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

i32 get_scale(Board& board, i32 eval);

};