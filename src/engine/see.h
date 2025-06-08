#pragma once

#include "eval.h"

namespace see
{

inline bool is_ok(Board& board, const u16& move, i32 threshold)
{
    // Skips special moves
    if (move::get_type(move) != move::type::NORMAL) {
        return true;
    }

    // Move data
    auto from = move::get_from(move);
    auto to = move::get_to(move);

    auto piece = board.get_type_at(from);
    auto captured = board.get_type_at(to);

    // Piece balance
    i32 value = (captured == piece::type::NONE ? 0 : eval::PIECE_VALUE[captured]) - threshold;

    // If we still lose after making the move, then stop
    if (value < 0) {
        return false;
    }

    // If we still win after losing the piece, then stop
    value -= eval::PIECE_VALUE[piece];

    if (value >= 0) {
        return true;
    }

    // Occupied pieces
    u64 occupied = board.get_occupied() ^ bitboard::create(from);

    // Gets attackers
    u64 attackers = board.get_attackers(to, occupied);
    u64 bishops = board.get_pieces(piece::type::BISHOP) | board.get_pieces(piece::type::QUEEN);
    u64 rooks = board.get_pieces(piece::type::ROOK) | board.get_pieces(piece::type::QUEEN);

    // Side to move
    i8 color = !board.get_color();

    // Gets pinned pieces
    const u64 pinned_white = board.get_blockers(color::WHITE) & board.get_colors(color::WHITE);
    const u64 pinned_black = board.get_blockers(color::BLACK) & board.get_colors(color::BLACK);
    const u64 pinned = pinned_white | pinned_black;

    // Gets movable ray mask for pinned pieces
    const u64 ray_white = bitboard::get_between(board.get_king_square(color::WHITE), to) | bitboard::create(to);
    const u64 ray_black = bitboard::get_between(board.get_king_square(color::BLACK), to) | bitboard::create(to);

    // Gets movable pieces
    attackers &= ~pinned | (pinned_white & ray_white) | (pinned_black & ray_black);

    // Makes captures until one side runs out or loses
    while (true)
    {
        // Removes used piece from the attackers
        attackers &= occupied;

        // Checks if we run out of captures to make
        u64 attackers_us = attackers & board.get_colors(color);

        if (!attackers_us) {
            break;
        }

        // Picks next least valuable piece to capture with
        i8 pt;

        for (pt = piece::type::PAWN; pt < piece::type::KING; ++pt) {
            if (attackers_us & board.get_pieces(pt)) {
                break;
            }
        }

        // Flips side to move
        color = !color;
        value = -value - 1 - eval::PIECE_VALUE[pt];

        // Negamax
        if (value >= 0) {
            if (pt == piece::type::KING && (attackers & board.get_colors(color))) {
                color = !color;
            }

            break;
        }

        // Removes used piece from occupied
        occupied ^= bitboard::create(bitboard::get_lsb(attackers_us & board.get_pieces(pt)));

        // Adds possible discovered attacks
        if (pt == piece::type::PAWN || pt == piece::type::BISHOP || pt == piece::type::QUEEN) {
            attackers |= attack::get_bishop(to, occupied) & bishops;
        }

        if (pt == piece::type::ROOK || pt == piece::type::QUEEN) {
            attackers |= attack::get_rook(to, occupied) & rooks;
        }
    }
    
    return color != board.get_color();
};

};