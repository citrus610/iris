#pragma once

#include "attack.h"
#include "zobrist.h"
#include "bitboard.h"
#include "cuckoo.h"

#include "../util/arrayvec.h"

constexpr i32 MAX_PLY = 256;

struct Undo
{
    u64 hash;
    u64 hash_pawn;
    u64 hash_non_pawn[2];
    i8 castling;
    i8 enpassant;
    i8 captured;
    i32 halfmove;
    u64 checkers;
    u64 blockers[2];
    u64 threats;
};

class Board
{
public:
    static constexpr auto STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
private:
    u64 pieces[6];
    u64 colors[2];
    i8 board[64];
    i8 color;
    i8 castling;
    i8 enpassant;
    i32 halfmove;
    i32 ply;
private:
    u64 checkers;
    u64 blockers[2];
    u64 threats;
private:
    u64 hash;
    u64 hash_pawn;
    u64 hash_non_pawn[2];
private:
    std::vector<Undo> history;
public:
    Board(const std::string& fen = STARTPOS);
public:
    void set_fen(const std::string& fen);
public:
    u64 get_occupied();
    u64 get_pieces(i8 type);
    u64 get_pieces(i8 type, i8 color);
    u64 get_colors(i8 color);
    i8 get_color();
    i8 get_piece_at(i8 square);
    i8 get_type_at(i8 square);
    i8 get_color_at(i8 square);
    i8 get_castling_right();
    i8 get_enpassant_square();
    i32 get_halfmove_count();
    i32 get_fullmove_count();
    i32 get_ply();
    u64 get_checkers();
    u64 get_blockers(i8 color);
    u64 get_threats();
    u64 get_hash();
    u64 get_hash_pawn();
    u64 get_hash_non_pawn(i8 color);
    std::string get_fen();
public:
    i8 get_king_square(i8 color);
    i8 get_captured_type(u16 move);
    u64 get_attackers(i8 square, u64 occupied);
    u64 get_hash_after(u16 move);
    u64 get_hash_slow();
    u64 get_hash_pawn_slow();
    u64 get_hash_non_pawn_slow(i8 color);
public:
    bool is_draw(i32 search_ply = 0);
    bool is_draw_repitition(i32 search_ply = 0);
    bool is_draw_fifty_move();
    bool is_draw_insufficient();
    bool is_square_attacked(i8 square, i8 color, u64 occupied);
    bool is_pseudo_legal(u16 move);
    bool is_legal(u16 move);
    bool is_quiet(u16 move);
    bool has_non_pawn(i8 color);
    bool has_upcomming_repetition();
public:
    void make(u16 move);
    void unmake(u16 move);
    void make_null();
    void unmake_null();
    void remove(i8 type, i8 color, i8 square);
    void place(i8 type, i8 color, i8 square);
    void update_masks();
    void update_checkers();
    template <i8 COLOR> void update_blockers();
    void update_threats();
    void print();
};

inline u64 Board::get_occupied()
{
    return this->colors[0] | this->colors[1];
};

inline u64 Board::get_pieces(i8 type)
{
    assert(piece::type::is_valid(type));

    return this->pieces[type];
};

inline u64 Board::get_pieces(i8 type, i8 color)
{
    assert(piece::type::is_valid(type));
    assert(piece::type::is_valid(color));

    return this->pieces[type] & this->colors[color];
};

inline u64 Board::get_colors(i8 color)
{
    assert(piece::type::is_valid(color));

    return this->colors[color];
};

inline i8 Board::get_color()
{
    return this->color;
};

inline i8 Board::get_piece_at(i8 square)
{
    assert(square::is_valid(square));

    return this->board[square];
};

inline i8 Board::get_type_at(i8 square)
{
    assert(square::is_valid(square));

    if (this->board[square] == piece::NONE) {
        return piece::type::NONE;
    }

    return piece::get_type(this->board[square]);
};

inline i8 Board::get_color_at(i8 square)
{
    assert(square::is_valid(square));

    if (this->board[square] == piece::NONE) {
        return color::NONE;
    }

    return piece::get_color(this->board[square]);
};

inline i8 Board::get_castling_right()
{
    return this->castling;
};

inline i8 Board::get_enpassant_square()
{
    return this->enpassant;
};

inline i32 Board::get_halfmove_count()
{
    return this->halfmove;
};

inline i32 Board::get_fullmove_count()
{
    return this->ply / 2 + 1;
};

inline i32 Board::get_ply()
{
    return this->ply;
};

inline u64 Board::get_checkers()
{
    return this->checkers;
};

inline u64 Board::get_blockers(i8 color)
{
    assert(color::is_valid(color));

    return this->blockers[color];
};

inline u64 Board::get_threats()
{
    return this->threats;
};

inline u64 Board::get_hash()
{
    return this->hash;
};

inline u64 Board::get_hash_pawn()
{
    return this->hash_pawn;
};

inline u64 Board::get_hash_non_pawn(i8 color)
{
    assert(color::is_valid(color));

    return this->hash_non_pawn[color];
};

inline void Board::update_checkers()
{
    const u64 occupied = this->get_occupied();

    const u64 pawns = this->pieces[piece::type::PAWN];
    const u64 knights = this->pieces[piece::type::KNIGHT];
    const u64 bishops = this->pieces[piece::type::BISHOP] | this->pieces[piece::type::QUEEN];
    const u64 rooks = this->pieces[piece::type::ROOK] | this->pieces[piece::type::QUEEN];

    const i8 king_square = this->get_king_square(this->color);

    this->checkers = 0ULL;

    this->checkers |= attack::get_pawn(king_square, this->color) & pawns;
    this->checkers |= attack::get_knight(king_square) & knights;
    this->checkers |= attack::get_bishop(king_square, occupied) & bishops;
    this->checkers |= attack::get_rook(king_square, occupied) & rooks;

    this->checkers &= this->colors[!this->color];
};

template <i8 COLOR>
inline void Board::update_blockers()
{
    this->blockers[COLOR] = 0;

    const i8 king_square = this->get_king_square(COLOR);

    const u64 enemy = this->colors[!COLOR];
    const u64 bishop = this->pieces[piece::type::BISHOP] | this->pieces[piece::type::QUEEN];
    const u64 rook = this->pieces[piece::type::ROOK] | this->pieces[piece::type::QUEEN];

    u64 snipers = ((attack::get_bishop(king_square, 0) & bishop) | (attack::get_rook(king_square, 0) & rook)) & enemy;
    u64 occupied = this->get_occupied() ^ snipers;

    while (snipers)
    {
        i8 square = bitboard::pop_lsb(snipers);

        u64 ray = bitboard::get_between(king_square, square) & occupied;

        if (bitboard::get_count(ray) == 1) {
            this->blockers[COLOR] |= ray;
        }
    }
};

inline void Board::update_threats()
{
    const u64 occupied = this->get_occupied();
    const u64 enemy = this->colors[!this->color];

    u64 enemy_pawns = this->pieces[piece::type::PAWN] & enemy;
    u64 enemy_knights = this->pieces[piece::type::KNIGHT] & enemy;
    u64 enemy_bishops = (this->pieces[piece::type::BISHOP] | this->pieces[piece::type::QUEEN]) & enemy;
    u64 enemy_rooks = (this->pieces[piece::type::ROOK] | this->pieces[piece::type::QUEEN]) & enemy;

    this->threats = attack::get_king(this->get_king_square(!this->color));

    if (this->color == color::WHITE) {
        this->threats |= attack::get_pawn_span<color::BLACK>(enemy_pawns);
    }
    else {
        this->threats |= attack::get_pawn_span<color::WHITE>(enemy_pawns);
    }

    while (enemy_knights)
    {
        this->threats |= attack::get_knight(bitboard::pop_lsb(enemy_knights));
    }

    while (enemy_bishops)
    {
        this->threats |= attack::get_bishop(bitboard::pop_lsb(enemy_bishops), occupied);
    }

    while (enemy_rooks)
    {
        this->threats |= attack::get_rook(bitboard::pop_lsb(enemy_rooks), occupied);
    }
};