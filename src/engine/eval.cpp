#include "eval.h"

namespace eval
{

i32 get(Board& board)
{
    i32 score = 0;

    // Evaluates
    score += eval::get_material(board);
    score += eval::get_table(board);
    score += eval::get_mobility(board);
    score += eval::get_king_defense(board);
    score += eval::get_bishop_pair(board);
    score += eval::get_pawn_structure(board);
    score += eval::get_threat(board);
    score += eval::get_open(board);

    // Gets midgame and engame values
    i32 midgame = score::get_midgame(score);
    i32 endgame = score::get_endgame(score);

    // Calculates phase
    i32 phase = PHASE_MAX;

    phase -= bitboard::get_count(board.get_pieces(piece::type::KNIGHT)) * PHASE_KNIGHT;
    phase -= bitboard::get_count(board.get_pieces(piece::type::BISHOP)) * PHASE_BISHOP;
    phase -= bitboard::get_count(board.get_pieces(piece::type::ROOK)) * PHASE_ROOK;
    phase -= bitboard::get_count(board.get_pieces(piece::type::QUEEN)) * PHASE_QUEEN;

    phase = phase * PHASE_SCALE / PHASE_MAX;

    // Gets endgame scale
    i32 scale = eval::get_scale(board, score);

    // Mixes midgame and endgame values
    score = (midgame * (PHASE_SCALE - phase) + endgame * phase * scale / SCALE_MAX) / PHASE_SCALE;

    // Returns score based on side to move with tempo
    return (board.get_color() == color::WHITE ? score : -score) + eval::DEFAULT.tempo;
};

i32 get_material(Board& board)
{
    const u64 white = board.get_colors(color::WHITE);
    const u64 black = board.get_colors(color::BLACK);

    const u64 pawn = board.get_pieces(piece::type::PAWN);
    const u64 knight = board.get_pieces(piece::type::KNIGHT);
    const u64 bishop = board.get_pieces(piece::type::BISHOP);
    const u64 rook = board.get_pieces(piece::type::ROOK);
    const u64 queen = board.get_pieces(piece::type::QUEEN);
    const u64 king = board.get_pieces(piece::type::KING);

    i32 dt_pawn = bitboard::get_count(pawn & white) - bitboard::get_count(pawn & black);
    i32 dt_knight = bitboard::get_count(knight & white) - bitboard::get_count(knight & black);
    i32 dt_bishop = bitboard::get_count(bishop & white) - bitboard::get_count(bishop & black);
    i32 dt_rook = bitboard::get_count(rook & white) - bitboard::get_count(rook & black);
    i32 dt_queen = bitboard::get_count(queen & white) - bitboard::get_count(queen & black);
    i32 dt_king = bitboard::get_count(king & white) - bitboard::get_count(king & black);

    i32 material = 0;

    material += dt_pawn * eval::DEFAULT.material[0];
    material += dt_knight * eval::DEFAULT.material[1];
    material += dt_bishop * eval::DEFAULT.material[2];
    material += dt_rook * eval::DEFAULT.material[3];
    material += dt_queen * eval::DEFAULT.material[4];
    material += dt_king * eval::DEFAULT.material[5];

    return material;
};

i32 get_table(Board& board)
{
    i32 table[2] = { 0, 0 };

    for (i8 square = 0; square < 64; ++square) {
        i8 piece = board.get_piece_at(square);

        if (piece == piece::NONE) {
            continue;
        }

        i8 type = piece::get_type(piece);
        i8 color = piece::get_color(piece);

        table[color] += eval::DEFAULT.table[color][type][square];
    }

    return table[0] - table[1];
};

i32 get_mobility(Board& board)
{
    const u64 colors[2] = {
        board.get_colors(color::WHITE),
        board.get_colors(color::BLACK)
    };

    const u64 occupied = colors[0] | colors[1];

    const u64 moveable[2] = {
        ~(colors[color::WHITE] | attack::get_pawn_span<color::BLACK>(board.get_pieces(piece::type::PAWN, color::BLACK))),
        ~(colors[color::BLACK] | attack::get_pawn_span<color::WHITE>(board.get_pieces(piece::type::PAWN, color::WHITE)))
    };

    i32 mobility[2] = { 0, 0 };

    for (i8 square = 0; square < 64; ++square) {
        i8 piece = board.get_piece_at(square);

        if (piece == piece::NONE) {
            continue;
        }

        i8 type = piece::get_type(piece);
        i8 color = piece::get_color(piece);

        if (type == piece::type::KNIGHT) {
            u64 attack = attack::get_knight(square);
            
            mobility[color] += eval::DEFAULT.mobility_knight[bitboard::get_count(attack & moveable[color])];
        }
        else if (type == piece::type::BISHOP) {
            u64 attack = attack::get_bishop(square, occupied);

            mobility[color] += eval::DEFAULT.mobility_bishop[bitboard::get_count(attack & moveable[color])];
        }
        else if (type == piece::type::ROOK) {
            u64 attack = attack::get_rook(square, occupied);

            mobility[color] += eval::DEFAULT.mobility_rook[bitboard::get_count(attack & moveable[color])];
        }
        else if (type == piece::type::QUEEN) {
            u64 attack = attack::get_bishop(square, occupied) | attack::get_rook(square, occupied);

            mobility[color] += eval::DEFAULT.mobility_queen[bitboard::get_count(attack & moveable[color])];
        }
    }

    return mobility[0] - mobility[1];
};

i32 get_king_defense(Board& board)
{
    i32 defense[2] = { 0, 0 };

    const u64 minor =
        board.get_pieces(piece::type::PAWN) |
        board.get_pieces(piece::type::KNIGHT) |
        board.get_pieces(piece::type::BISHOP);

    for (i8 color = color::WHITE; color < 2; ++color) {
        const i8 king_square = board.get_king_square(color);
        const u64 defender = minor & board.get_colors(color) & attack::get_king(king_square);
        const i32 count = bitboard::get_count(defender);

        defense[color] += eval::DEFAULT.king_defense[count];
    }

    return defense[0] - defense[1];
};

i32 get_bishop_pair(Board& board)
{
    i32 bishop_pair = 0;

    const u64 bishop_white = board.get_pieces(piece::type::BISHOP, color::WHITE);
    const u64 bishop_black = board.get_pieces(piece::type::BISHOP, color::BLACK);

    if (bitboard::is_many(bishop_white) && (bishop_white & bitboard::LIGHT) && (bishop_white & bitboard::DARK)) {
        bishop_pair += eval::DEFAULT.bishop_pair;
    }

    if (bitboard::is_many(bishop_black) && (bishop_black & bitboard::LIGHT) && (bishop_black & bitboard::DARK)) {
        bishop_pair -= eval::DEFAULT.bishop_pair;
    }

    return bishop_pair;
};

i32 get_pawn_structure(Board& board)
{
    // Passed pawns mask
    constexpr std::array<std::array<u64, 64>, 2> FORWARD_PASS = [] {
        std::array<std::array<u64, 64>, 2> result = { 0ULL };

        for (i8 color = 0; color < 2; ++color) {
            for (i8 sq = 0; sq < 64; ++sq) {
                u64 mask = bitboard::create(sq);

                if (color == color::WHITE) {
                    mask |= attack::get_pawn_left<color::WHITE>(bitboard::create(sq));
                    mask |= attack::get_pawn_right<color::WHITE>(bitboard::create(sq));
                    mask |= bitboard::get_fill_up(mask);
                }
                else {
                    mask |= attack::get_pawn_left<color::BLACK>(bitboard::create(sq));
                    mask |= attack::get_pawn_right<color::BLACK>(bitboard::create(sq));
                    mask |= bitboard::get_fill_down(mask);
                }

                result[color][sq] = mask;
            }
        }
    
        return result;
    } ();

    // Isolated pawns mask
    constexpr std::array<u64, 8> ADJACENT = [] {
        std::array<u64, 8> result = { 0ULL };

        for (i8 file = 0; file < 8; ++file) {
            if (file > 0) {
                result[file] |= bitboard::FILE[file - 1];
            }

            if (file < 7) {
                result[file] |= bitboard::FILE[file + 1];
            }
        }
    
        return result;
    } ();

    i32 pawn_structure[2] = { 0, 0 };

    // Passed pawns
    for (i8 color = 0; color < 2; ++color) {
        u64 pawns_us = board.get_pieces(piece::type::PAWN, color);
        u64 pawns_them = board.get_pieces(piece::type::PAWN, !color);

        while (pawns_us)
        {
            i8 sq = bitboard::pop_lsb(pawns_us);

            if ((FORWARD_PASS[color][sq] & pawns_them) == 0) {
                pawn_structure[color] += eval::DEFAULT.pawn_passed[rank::get_relative(square::get_rank(sq), color)];
            }
        }
    }

    // Pawns phalanx
    for (i8 color = 0; color < 2; ++color) {
        const u64 pawns = board.get_pieces(piece::type::PAWN, color);
        u64 phalanx = pawns & bitboard::get_shift<direction::WEST>(pawns);

        while (phalanx)
        {
            const i8 sq = bitboard::pop_lsb(phalanx);

            pawn_structure[color] += eval::DEFAULT.pawn_phalanx[rank::get_relative(square::get_rank(sq), color)];
        }
    }

    // Isolated pawns
    for (i8 color = 0; color < 2; ++color) {
        const u64 pawns = board.get_pieces(piece::type::PAWN, color);

        for (i8 file = 0; file < 8; ++file) {
            if (ADJACENT[file] & pawns) {
                continue;
            }

            pawn_structure[color] += bitboard::get_count(pawns & bitboard::FILE[file]) * eval::DEFAULT.pawn_isolated;
        }
    }

    // Protected pawns
    const u64 pawns_white = board.get_pieces(piece::type::PAWN, color::WHITE);
    const u64 pawns_black = board.get_pieces(piece::type::PAWN, color::BLACK);

    const u64 pawns_attack_white = attack::get_pawn_span<color::WHITE>(pawns_white);
    const u64 pawns_attack_black = attack::get_pawn_span<color::BLACK>(pawns_black);

    pawn_structure[0] += bitboard::get_count(pawns_white & pawns_attack_white) * eval::DEFAULT.pawn_protected;
    pawn_structure[1] += bitboard::get_count(pawns_black & pawns_attack_black) * eval::DEFAULT.pawn_protected;

    return pawn_structure[0] - pawn_structure[1];
};

i32 get_threat(Board& board)
{
    i32 threat = 0;

    const u64 pawns = board.get_pieces(piece::type::PAWN);
    const u64 knights = board.get_pieces(piece::type::KNIGHT);
    const u64 bishops = board.get_pieces(piece::type::BISHOP);
    const u64 rooks = board.get_pieces(piece::type::ROOK);
    const u64 queens = board.get_pieces(piece::type::QUEEN);

    const u64 white = board.get_colors(color::WHITE);
    const u64 black = board.get_colors(color::BLACK);

    // Pawn threat
    const u64 attack_pawn_white = attack::get_pawn_span<color::WHITE>(pawns & white);
    const u64 attack_pawn_black = attack::get_pawn_span<color::BLACK>(pawns & black);

    const i32 threat_pawn_minor_white = bitboard::get_count(attack_pawn_white & (knights | bishops) & black);
    const i32 threat_pawn_minor_black = bitboard::get_count(attack_pawn_black & (knights | bishops) & white);

    const i32 threat_pawn_rook_white = bitboard::get_count(attack_pawn_white & rooks & black);
    const i32 threat_pawn_rook_black = bitboard::get_count(attack_pawn_black & rooks & white);

    const i32 threat_pawn_queen_white = bitboard::get_count(attack_pawn_white & queens & black);
    const i32 threat_pawn_queen_black = bitboard::get_count(attack_pawn_black & queens & white);

    threat += (threat_pawn_minor_white - threat_pawn_minor_black) * eval::DEFAULT.threat_pawn[0];
    threat += (threat_pawn_rook_white - threat_pawn_rook_black) * eval::DEFAULT.threat_pawn[1];
    threat += (threat_pawn_queen_white - threat_pawn_queen_black) * eval::DEFAULT.threat_pawn[2];

    return threat;
};

i32 get_open(Board& board)
{
    i32 result = 0;

    const u64 pawn_white = board.get_pieces(piece::type::PAWN, color::WHITE);
    const u64 pawn_black = board.get_pieces(piece::type::PAWN, color::BLACK);

    const u64 semiopen_white = ~(bitboard::get_fill_up(pawn_white) | bitboard::get_fill_down(pawn_white));
    const u64 semiopen_black = ~(bitboard::get_fill_up(pawn_black) | bitboard::get_fill_down(pawn_black));

    const u64 open = semiopen_white & semiopen_black;

    // King
    const u64 king_white = board.get_pieces(piece::type::KING, color::WHITE);
    const u64 king_black = board.get_pieces(piece::type::KING, color::BLACK);

    result += bitboard::get_count(king_white & open) * eval::DEFAULT.king_open;
    result += bitboard::get_count(king_white & semiopen_white) * eval::DEFAULT.king_semiopen;

    result -= bitboard::get_count(king_black & open) * eval::DEFAULT.king_open;
    result -= bitboard::get_count(king_black & semiopen_black) * eval::DEFAULT.king_semiopen;

    return result;
};

i32 get_scale(Board& board, i32 eval)
{
    const i8 strong = eval > 0 ? color::WHITE : color::BLACK;
    const u64 strong_pawn = board.get_pieces(piece::type::PAWN, strong);
    const i32 strong_pawn_count = bitboard::get_count(strong_pawn);
    const i32 x = 8 - strong_pawn_count;

    return SCALE_MAX - x * x;
};

i32 get_adjusted(i32 eval, i32 correction, i32 halfmove)
{
    return std::clamp(eval * (200 - halfmove) / 200 + correction, -score::MATE_FOUND + 1, score::MATE_FOUND - 1);
};

};