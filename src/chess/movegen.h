#pragma once

#include "board.h"

namespace move::gen
{

enum class type
{
    ALL,
    QUIET,
    NOISY
};

inline void add_normals(arrayvec<u16, move::MAX>& list, i8 from, u64 targets)
{
    while (targets)
    {
        list.add(move::get<move::type::NORMAL>(from, bitboard::pop_lsb(targets)));
    }
};

inline void add_promotions(arrayvec<u16, move::MAX>& list, i8 from, i8 to)
{
    list.add(move::get<move::type::PROMOTION>(from, to, piece::type::KNIGHT));
    list.add(move::get<move::type::PROMOTION>(from, to, piece::type::BISHOP));
    list.add(move::get<move::type::PROMOTION>(from, to, piece::type::ROOK));
    list.add(move::get<move::type::PROMOTION>(from, to, piece::type::QUEEN));
};

template <i8 COLOR, move::gen::type TYPE>
inline void add_pawns(Board& board, arrayvec<u16, move::MAX>& list, u64 check_mask)
{
    constexpr i8 UP = direction::get<COLOR>(direction::NORTH);
    constexpr i8 UP_LEFT = direction::get<COLOR>(direction::NORTH_WEST);
    constexpr i8 UP_RIGHT = direction::get<COLOR>(direction::NORTH_EAST);

    constexpr u64 MASK_PUSH_1 = COLOR == color::WHITE ? bitboard::RANK_3 : bitboard::RANK_6;
    constexpr u64 MASK_PROMOTION = COLOR == color::WHITE ? bitboard::RANK_8 : bitboard::RANK_1;

    const u64 empty = ~board.get_occupied();
    const u64 enemy = board.get_colors(!COLOR);
    const u64 pawns = board.get_pieces(piece::type::PAWN, COLOR);

    // Push moves
    u64 up_1 = bitboard::get_shift<UP>(pawns) & empty;
    u64 up_2 = bitboard::get_shift<UP>(up_1 & MASK_PUSH_1) & empty;

    up_1 &= check_mask;
    up_2 &= check_mask;

    // Capture moves
    u64 left = attack::get_pawn_left<COLOR>(pawns) & enemy & check_mask;
    u64 right = attack::get_pawn_right<COLOR>(pawns) & enemy & check_mask;

    // Adds promotion moves
    if constexpr (TYPE != move::gen::type::QUIET) {
        u64 promo_up = up_1 & MASK_PROMOTION;
        u64 promo_left = left & MASK_PROMOTION;
        u64 promo_right = right & MASK_PROMOTION;

        while (promo_up)
        {
            i8 to = bitboard::pop_lsb(promo_up);
            move::gen::add_promotions(list, to - UP, to);
        }

        while (promo_left)
        {
            i8 to = bitboard::pop_lsb(promo_left);
            move::gen::add_promotions(list, to - UP_LEFT, to);
        }

        while (promo_right)
        {
            i8 to = bitboard::pop_lsb(promo_right);
            move::gen::add_promotions(list, to - UP_RIGHT, to);
        }
    }

    // Removes promotion moves
    up_1 &= ~MASK_PROMOTION;
    left &= ~MASK_PROMOTION;
    right &= ~MASK_PROMOTION;

    // Adds push moves
    while (TYPE != move::gen::type::NOISY && up_1)
    {
        i8 to = bitboard::pop_lsb(up_1);
        list.add(move::get<move::type::NORMAL>(to - UP, to));
    }

    while (TYPE != move::gen::type::NOISY && up_2)
    {
        i8 to = bitboard::pop_lsb(up_2);
        list.add(move::get<move::type::NORMAL>(to - UP - UP, to));
    }

    // Prunes noisy moves
    if constexpr (TYPE == move::gen::type::QUIET) {
        return;
    }

    // Adds capture moves
    while (left)
    {
        i8 to = bitboard::pop_lsb(left);
        list.add(move::get<move::type::NORMAL>(to - UP_LEFT, to));
    }

    while (right)
    {
        i8 to = bitboard::pop_lsb(right);
        list.add(move::get<move::type::NORMAL>(to - UP_RIGHT, to));
    }

    // Adds enpassant moves
    const i8 ep = board.get_enpassant_square();

    if (ep == square::NONE) {
        return;
    }

    u64 pawns_ep = attack::get_pawn(ep, !COLOR) & pawns;

    while (pawns_ep)
    {
        list.add(move::get<move::type::ENPASSANT>(bitboard::pop_lsb(pawns_ep), ep));
    }
};

template <i8 COLOR>
inline void add_castlings(Board& board, arrayvec<u16, move::MAX>& list)
{
    const i8 king_from = board.get_king_square(COLOR);
    const u64 occupied = board.get_occupied();

    i8 right = board.get_castling_right();

    if constexpr (COLOR == color::WHITE) {
        right &= castling::WHITE;
    }
    else {
        right &= castling::BLACK;
    }

    while (right)
    {
        const i8 castle = right & (-right);
        right &= right - 1;

        const i8 rook_from = castling::get_rook_from(COLOR, castle & castling::SHORT);

        if (bitboard::get_between(king_from, rook_from) & occupied) {
            continue;
        }

        list.add(move::get<move::type::CASTLING>(king_from, rook_from));
    }
    
};

template <i8 COLOR, move::gen::type TYPE>
inline arrayvec<u16, move::MAX> get(Board& board)
{
    auto list = arrayvec<u16, move::MAX>();

    const u64 us = board.get_colors(COLOR);
    const u64 them = board.get_colors(!COLOR);
    const u64 occupied = us | them;
    const u64 checkers = board.get_checkers();
    const u64 blockers = board.get_blockers(COLOR);

    // Moveable squares
    u64 movable = 
        TYPE == move::gen::type::ALL ? ~us :
        TYPE == move::gen::type::NOISY ? them :
        ~occupied;

    // King
    const i8 king_square = board.get_king_square(COLOR);

    move::gen::add_normals(list, king_square, attack::get_king(king_square) & movable);

    // Double check
    if (bitboard::is_many(checkers)) {
        return list;
    }

    // Gets check mask
    u64 check_mask = ~0ULL;

    if (checkers) {
        check_mask = checkers | bitboard::get_between(king_square, bitboard::get_lsb(checkers));
    }

    movable &= check_mask;

    // Castlings
    if (TYPE != move::gen::type::NOISY && !checkers) {
        move::gen::add_castlings<COLOR>(board, list);
    }

    // Pawns
    move::gen::add_pawns<COLOR, TYPE>(board, list, check_mask);

    // Knights
    u64 knights = board.get_pieces(piece::type::KNIGHT, COLOR) & ~blockers;

    while (knights)
    {
        i8 from = bitboard::pop_lsb(knights);
        move::gen::add_normals(list, from, attack::get_knight(from) & movable);
    }

    // Queens
    const u64 queens = board.get_pieces(piece::type::QUEEN, COLOR);

    // Bishops
    u64 bishops = board.get_pieces(piece::type::BISHOP, COLOR) | queens;

    while (bishops)
    {
        i8 from = bitboard::pop_lsb(bishops);
        u64 targets = attack::get_bishop(from, occupied) & movable;

        if (blockers & bitboard::create(from)) {
            targets &= bitboard::get_line(from, king_square);
        }

        move::gen::add_normals(list, from, targets);
    }

    // Rooks
    u64 rooks = board.get_pieces(piece::type::ROOK, COLOR) | queens;

    while (rooks)
    {
        i8 from = bitboard::pop_lsb(rooks);
        u64 targets = attack::get_rook(from, occupied) & movable;

        if (blockers & bitboard::create(from)) {
            targets &= bitboard::get_line(from, king_square);
        }

        move::gen::add_normals(list, from, targets);
    }

    return list;
};

template <move::gen::type TYPE>
inline arrayvec<u16, move::MAX> get(Board& board)
{
    if (board.get_color() == color::WHITE) {
        return move::gen::get<color::WHITE, TYPE>(board);
    }

    return move::gen::get<color::BLACK, TYPE>(board);
};

};