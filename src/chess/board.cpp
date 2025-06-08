#include "board.h"

Board::Board(const std::string& fen)
{
    this->set_fen(fen);
    this->history.reserve(512);
    this->update_masks();
};

void Board::set_fen(const std::string& fen)
{
    // Zero init
    for (i8 p = 0; p < 6; ++p) {
        this->pieces[p] = 0ULL;
    }

    for (i8 c = 0; c < 2; ++c) {
        this->colors[c] = 0ULL;
    }

    for (i8 sq = 0; sq < 64; ++sq) {
        this->board[sq] = piece::NONE;
    }
    
    this->color = color::WHITE;
    this->castling = castling::NONE;
    this->enpassant = square::NONE;
    this->halfmove = 0;
    this->ply = 0;
    this->hash = 0;

    // Reads fen
    std::stringstream ss(fen);
    std::string word;
    std::vector<std::string> data;

    while (std::getline(ss, word, ' '))
    {
        data.push_back(word);
    }

    assert(data.size() >= 4);
    assert(!data[0].empty());

    auto str_board = data.size() > 0 ? data[0] : "";
    auto str_color = data.size() > 1 ? data[1] : "w";
    auto str_castling = data.size() > 2 ? data[2] : "-";
    auto str_enpassant = data.size() > 3 ? data[3] : "-";
    auto str_halfmove = data.size() > 4 ? data[4] : "0";
    auto str_fullmove = data.size() > 5 ? data[5] : "1";

    // Sets board
    auto square = 56;

    for (char c : str_board) {
        if (std::isdigit(c)) {
            square += c - '0';
        }
        else if (c == '/') {
            square -= 16;
        }
        else {
            i8 piece = piece::create(c);
            i8 piece_type = piece::get_type(piece);
            i8 piece_color = piece::get_color(piece);

            this->board[square] = piece;
            this->pieces[piece_type] |= bitboard::create(square);
            this->colors[piece_color] |= bitboard::create(square);

            this->hash ^= zobrist::get_piece(piece, square);

            square += 1;
        }
    }

    // Sets color
    this->color = str_color == "w" ? color::WHITE : color::BLACK;

    if (this->color == color::WHITE) {
        this->hash ^= zobrist::get_color();
    }

    // Sets castling
    for (char c : str_castling) {
        this->castling |= (c == 'K') ? castling::WHITE_SHORT : castling::NONE;
        this->castling |= (c == 'Q') ? castling::WHITE_LONG : castling::NONE;
        this->castling |= (c == 'k') ? castling::BLACK_SHORT : castling::NONE;
        this->castling |= (c == 'q') ? castling::BLACK_LONG : castling::NONE;
    }

    if (this->castling) {
        this->hash ^= zobrist::get_castling(this->castling);
    }

    // Sets enpassant square
    if (str_enpassant == "-") {
        this->enpassant = square::NONE;
    }
    else {
        assert(str_enpassant.size() == 2);

        i8 enpassant_file = file::create(str_enpassant[0]);
        i8 enpassant_rank = rank::create(str_enpassant[1]);

        this->enpassant = square::create(enpassant_file, enpassant_rank);

        this->hash ^= zobrist::get_enpassant(enpassant_file);
    }

    // Sets move count
    this->halfmove = std::stoi(str_halfmove);
    this->ply = std::stoi(str_fullmove) * 2 - 2 + this->color;

    // Sets hash
    assert(this->hash = this->get_hash_slow());

    this->hash_pawn = this->get_hash_pawn_slow();
    this->hash_non_pawn[color::WHITE] = this->get_hash_non_pawn_slow(color::WHITE);
    this->hash_non_pawn[color::BLACK] = this->get_hash_non_pawn_slow(color::BLACK);
};

std::string Board::get_fen()
{
    std::string fen;

    for (i8 rank = 7; rank >= 0; --rank) {
        i32 space = 0;

        for (i8 file = 0; file < 8; ++file) {
            i8 square = square::create(file, rank);
            i8 piece = this->get_piece_at(square);

            if (piece != piece::NONE) {
                if (space) {
                    fen += std::to_string(space);
                    space = 0;
                }

                fen += piece::get_char(piece);
            }
            else {
                space += 1;
            }
        }

        if (space) {
            fen += std::to_string(space);
        }

        if (rank > 0) {
            fen += "/";
        }
    }

    fen += " ";
    fen += color::get_char(this->color);

    if (this->castling == castling::NONE) {
        fen += " -";
    }
    else {
        fen += " ";
        fen += (this->castling & castling::WHITE_SHORT) ? "K" : "";
        fen += (this->castling & castling::WHITE_LONG) ? "Q" : "";
        fen += (this->castling & castling::BLACK_SHORT) ? "k" : "";
        fen += (this->castling & castling::BLACK_LONG) ? "q" : "";
    }

    if (this->enpassant == square::NONE) {
        fen += " -";
    }
    else {
        fen += " ";
        fen += file::get_char(square::get_file(this->enpassant));
        fen += rank::get_char(square::get_rank(this->enpassant));
    }

    fen += " ";
    fen += std::to_string(this->halfmove);
    fen += " ";
    fen += std::to_string(this->get_fullmove_count());

    return fen;
};

i8 Board::get_king_square(i8 color)
{
    assert(color::is_valid(color));
    assert(this->get_pieces(piece::type::KING, color));

    return bitboard::get_lsb(this->get_pieces(piece::type::KING, color));
};

i8 Board::get_captured_type(u16 move)
{
    assert(!this->is_quiet(move));

    i8 captured = move::get_type(move) == move::type::ENPASSANT ? piece::type::PAWN : this->get_type_at(move::get_to(move));

    if (captured == piece::type::NONE) {
        captured = piece::type::PAWN;
    }

    return captured;
};

u64 Board::get_attackers(i8 square, u64 occupied)
{
    return
        (attack::get_pawn(square, color::WHITE) & this->colors[color::BLACK] & this->pieces[piece::type::PAWN]) |
        (attack::get_pawn(square, color::BLACK) & this->colors[color::WHITE] & this->pieces[piece::type::PAWN]) |
        (attack::get_knight(square) & this->pieces[piece::type::KNIGHT]) |
        (attack::get_bishop(square, occupied) & (this->pieces[piece::type::BISHOP] | this->pieces[piece::type::QUEEN])) |
        (attack::get_rook(square, occupied) & (this->pieces[piece::type::ROOK] | this->pieces[piece::type::QUEEN])) |
        (attack::get_king(square) & this->pieces[piece::type::KING]);
};

u64 Board::get_hash_slow()
{
    u64 result = 0ULL;

    for (i8 square = 0; square < 64; ++square) {
        const auto piece = this->get_piece_at(square);

        if (piece != piece::NONE) {
            result ^= zobrist::get_piece(piece, square);
        }
    }

    if (this->color == color::WHITE) {
        result ^= zobrist::get_color();
    }

    if (this->castling) {
        result ^= zobrist::get_castling(this->castling);
    }

    if (this->enpassant != square::NONE) {
        result ^= zobrist::get_enpassant(square::get_file(this->enpassant));
    }

    return result;
};

u64 Board::get_hash_pawn_slow()
{
    u64 result = 0ULL;

    for (i8 square = 0; square < 64; ++square) {
        const auto piece = this->get_piece_at(square);

        if (piece == piece::WHITE_PAWN || piece == piece::BLACK_PAWN) {
            result ^= zobrist::get_piece(piece, square);
        }
    }

    return result;
};

u64 Board::get_hash_non_pawn_slow(i8 color)
{
    u64 result = 0ULL;

    for (i8 square = 0; square < 64; ++square) {
        const auto piece = this->get_piece_at(square);
        const auto side = this->get_color_at(square);
        const auto type = this->get_type_at(square);

        if (piece == piece::NONE || side != color || type == piece::type::PAWN) {
            continue;
        }

        result ^= zobrist::get_piece(piece::create(type, color), square);
    }

    return result;
};

bool Board::is_draw(i32 search_ply)
{
    return this->is_draw_insufficient() || this->is_draw_repitition(search_ply) || this->is_draw_fifty_move();
};

bool Board::is_draw_repitition(i32 search_ply)
{
    i32 count = 0;
    i32 size = static_cast<i32>(this->history.size());

    for (i32 i = 4; i < this->halfmove + 2 && i <= size; i += 2) {
        if (this->history[size - i].hash != this->hash) {
            continue;
        }

        if (i <= search_ply) {
            return true;
        }

        count += 1;

        if (count == 2) {
            return true;
        }
    }

    return false;
};

bool Board::is_draw_fifty_move()
{
    if (this->halfmove < 100) {
        return false;
    }

    if (this->checkers == 0ULL) {
        return true;
    }

    return false;
};

bool Board::is_draw_insufficient()
{
    i32 count = bitboard::get_count(this->get_occupied());

    if (count == 2) {
        return true;
    }

    if (count == 3) {
        if (this->pieces[piece::type::KNIGHT] || this->pieces[piece::type::BISHOP]) {
            return true;
        }
    }

    if (count == 4) {
        if (bitboard::is_many(this->pieces[piece::type::BISHOP]) &&
            square::is_same_color(bitboard::get_lsb(this->pieces[piece::type::BISHOP]), bitboard::get_msb(this->pieces[piece::type::BISHOP]))) {
            return true;
        }
    }

    return false;
};

bool Board::is_square_attacked(i8 square, i8 color, u64 occupied)
{
    assert(square::is_valid(square));
    assert(color::is_valid(color));

    const u64 enemy = this->colors[!color];

    const u64 enemy_pawns = enemy & this->pieces[piece::type::PAWN];

    if (attack::get_pawn(square, color) & enemy_pawns) {
        return true;
    }

    const u64 enemy_knights = enemy & this->pieces[piece::type::KNIGHT];

    if (attack::get_knight(square) & enemy_knights) {
        return true;
    }

    const u64 enemy_bishops = enemy & (this->pieces[piece::type::BISHOP] | this->pieces[piece::type::QUEEN]);

    if (attack::get_bishop(square, occupied) & enemy_bishops) {
        return true;
    }

    const u64 enemy_rooks = enemy & (this->pieces[piece::type::ROOK] | this->pieces[piece::type::QUEEN]);

    if (attack::get_rook(square, occupied) & enemy_rooks) {
        return true;
    }

    const u64 enemy_kings = enemy & this->pieces[piece::type::KING];

    return attack::get_king(square) & enemy_kings;
};

bool Board::is_pseudo_legal(u16 move)
{
    if (move == move::NONE) {
        return false;
    }

    const u64 occupied = this->get_occupied();
    const u64 empty = ~occupied;

    const u16 move_type = move::get_type(move);
    const i8 from = move::get_from(move);
    const i8 to = move::get_to(move);
    const i8 piece = this->board[from];

    // Invalid move
    if (piece == piece::NONE || piece::get_color(piece) != this->color) {
        return false;
    }

    if (move_type != move::type::CASTLING && (this->colors[this->color] & bitboard::create(to))) {
        return false;
    }

    // Multiple checkers
    if (bitboard::is_many(this->checkers)) {
        return
            move_type == move::type::NORMAL &&
            piece::get_type(piece) == piece::type::KING &&
            (bitboard::create(to) & attack::get_king(from));
    }

    // Move type check
    if (move_type == move::type::CASTLING) {
        return
            !this->checkers &&
            (castling::create(to) & this->castling) &&
            !(bitboard::get_between(from, to) & occupied);
    }

    if (move_type == move::type::ENPASSANT) {
        return
            piece::get_type(piece) == piece::type::PAWN &&
            to == this->enpassant &&
            (attack::get_pawn(from, this->color) & bitboard::create(to));
    }

    if (move_type == move::type::PROMOTION && piece::get_type(piece) != piece::type::PAWN) {
        return false;
    }

    // King
    if (piece::get_type(piece) == piece::type::KING) {
        return attack::get_king(from) & bitboard::create(to);
    }

    // Blocking check
    if (this->checkers && !((bitboard::get_between(this->get_king_square(this->color), bitboard::get_lsb(this->checkers)) | this->checkers) & bitboard::create(to))) {
        return false;
    }

    // Pawn
    if (piece::get_type(piece) == piece::type::PAWN) {
        u64 span = 0ULL;

        if (color == color::WHITE) {
            u64 push_1 = bitboard::get_shift<direction::NORTH>(bitboard::create(from)) & empty;
            u64 push_2 = bitboard::get_shift<direction::NORTH>(push_1 & bitboard::RANK_3) & empty;
            u64 capture = attack::get_pawn(from, color::WHITE) & this->colors[color::BLACK];

            span = push_1 | push_2 | capture;
        }
        else {
            u64 push_1 = bitboard::get_shift<direction::SOUTH>(bitboard::create(from)) & empty;
            u64 push_2 = bitboard::get_shift<direction::SOUTH>(push_1 & bitboard::RANK_6) & empty;
            u64 capture = attack::get_pawn(from, color::BLACK) & this->colors[color::WHITE];

            span = push_1 | push_2 | capture;
        }

        // Checks promotion
        if (move_type != move::type::PROMOTION) {
            span &= ~(bitboard::RANK_1 | bitboard::RANK_8);
        }

        return span & bitboard::create(to);
    }

    // Pinned
    if ((this->blockers[this->color] & bitboard::create(from)) && !(bitboard::get_line(from, to) & this->get_pieces(piece::type::KING, this->color))) {
        return false;
    }

    // Other pieces
    u64 attack = 0ULL;

    switch (piece::get_type(piece))
    {
    case piece::type::KNIGHT:
        attack = attack::get_knight(from);
        break;
    case piece::type::BISHOP:
        attack = attack::get_bishop(from, occupied);
        break;
    case piece::type::ROOK:
        attack = attack::get_rook(from, occupied);
        break;
    case piece::type::QUEEN:
        attack = attack::get_bishop(from, occupied) | attack::get_rook(from, occupied);
        break;
    default:
        break;
    }

    return attack & bitboard::create(to);
};

bool Board::is_legal(u16 move)
{
    const u16 move_type = move::get_type(move);
    const i8 from = move::get_from(move);
    const i8 to = move::get_to(move);
    const i8 piece = this->board[from];

    // Skips for non-pawn pieces since we've already checked in movegen
    if (piece::get_type(piece) != piece::type::KING && piece::get_type(piece) != piece::type::PAWN) {
        return true;
    }

    // Castling
    if (move_type == move::type::CASTLING) {
        const i8 king_to = castling::get_king_to(this->color, to > from);
        const i8 rook_to = castling::get_rook_to(this->color, to > from);

        return
            !this->is_square_attacked(king_to, this->color, this->get_occupied()) &&
            !this->is_square_attacked(rook_to, this->color, this->get_occupied());
    }

    // King
    if (piece::get_type(piece) == piece::type::KING) {
        return !this->is_square_attacked(to, this->color, this->get_occupied() ^ bitboard::create(from));
    }

    // Enpassant
    if (move_type == move::type::ENPASSANT) {
        const i8 DOWN = this->color == color::WHITE ? direction::SOUTH : direction::NORTH;

        const u64 occupied = this->get_occupied() ^ bitboard::create(from) ^ bitboard::create(to) ^ bitboard::create(this->get_enpassant_square() + DOWN);

        const u64 enemy_queen = this->get_pieces(piece::type::QUEEN, !this->color);
        const u64 enemy_bishop = this->get_pieces(piece::type::BISHOP, !this->color) | enemy_queen;
        const u64 enemy_rook = this->get_pieces(piece::type::ROOK, !this->color) | enemy_queen;

        const i8 king_square = this->get_king_square(this->color);

        return
            !(attack::get_bishop(king_square, occupied) & enemy_bishop) &&
            !(attack::get_rook(king_square, occupied) & enemy_rook);
    }

    return !(this->blockers[this->color] & bitboard::create(from)) || (bitboard::get_line(from, to) & this->get_pieces(piece::type::KING, this->color));
};

bool Board::is_quiet(u16 move)
{
    return
        (move::get_type(move) == move::type::CASTLING) ||
        (move::get_type(move) == move::type::NORMAL && this->board[move::get_to(move)] == piece::NONE);
};

bool Board::has_non_pawn(i8 color)
{
    return this->colors[this->color] & ~(this->pieces[piece::type::PAWN] | this->pieces[piece::type::KING]);
};

bool Board::has_upcomming_repetition(i32 search_ply)
{
    const i32 size = static_cast<i32>(this->history.size());
    const i32 max = std::min(this->halfmove, size);

    u64 other = ~(this->hash ^ this->history[size - 1].hash);

    for (i32 i = 3; i <= max; i += 2) {
        other ^= ~(this->history[size - i].hash ^ this->history[size - i + 1].hash);

        if (other) {
            continue;
        }

        u64 hash = this->hash ^ this->history[size - i].hash;
        u64 index = cuckoo::get_h1(hash);

        if (cuckoo::HASH[index] != hash) {
            index = cuckoo::get_h2(hash);
        }

        if (cuckoo::HASH[index] != hash) {
            continue;
        }

        u16 move = cuckoo::MOVE[index];

        i8 a = move::get_from(move);
        i8 b = move::get_to(move);

        if (bitboard::get_between(a, b) & this->get_occupied()) {
            continue;
        }

        if (i < search_ply) {
            return true;
        }

        i8 piece = this->board[a];

        if (piece == piece::NONE) {
            piece = this->board[b];
        }

        assert(piece != piece::NONE);

        return piece::get_color(piece) == this->color;
    }

    return false;
};

void Board::make(u16 move)
{
    // Gets move data
    i8 move_from = move::get_from(move);
    i8 move_to = move::get_to(move);
    u16 move_type = move::get_type(move);

    i8 piece_type = this->get_type_at(move_from);
    i8 captured =
        move_type == move::type::CASTLING ?
        piece::type::NONE :
        this->get_type_at(move_to);

    // Validates
    assert(piece_type != piece::type::NONE);
    assert(this->get_color_at(move_from) == this->color);
    assert(this->get_color_at(move_to) != this->color || move_type == move::type::CASTLING);

    // Saves info
    this->history.push_back(Undo {
        .hash = this->hash,
        .hash_pawn = this->hash_pawn,
        .hash_non_pawn = {
            this->hash_non_pawn[0],
            this->hash_non_pawn[1]
        },
        .castling = this->castling,
        .enpassant = this->enpassant,
        .captured = captured,
        .halfmove = this->halfmove,
        .checkers = this->checkers,
        .blockers = {
            this->blockers[0],
            this->blockers[1]
        }
    });

    // Updates move counter
    this->halfmove += 1;
    this->ply += 1;

    // Removes enpassant square
    if (this->enpassant != square::NONE) {
        this->hash ^= zobrist::get_enpassant(square::get_file(this->enpassant));
        this->enpassant = square::NONE;
    }

    // Checks capture
    if (captured != piece::type::NONE) {
        // Updates half move counter
        this->halfmove = 0;

        // Removes piece
        this->remove(captured, !this->color, move_to);

        // Updates hash
        this->hash ^= zobrist::get_piece(piece::create(captured, !this->color), move_to);

        if (captured == piece::type::PAWN) {
            this->hash_pawn ^= zobrist::get_piece(piece::create(captured, !this->color), move_to);
        }
        else {
            this->hash_non_pawn[!this->color] ^= zobrist::get_piece(piece::create(captured, !this->color), move_to);
        }

        // Removes castling right if a rook is captured
        if (captured == piece::type::ROOK) {
            i8 castling_removed = castling::create(move_to) & this->castling;

            if (castling_removed) {
                this->castling ^= castling_removed;
                this->hash ^= zobrist::get_castling(castling_removed);
            }
        }
    }

    // Checks piece-specific actions
    if (piece_type == piece::type::KING) {
        // Removes castling rights
        i8 castling_removed =
            (this->color == color::WHITE) ?
            (this->castling & castling::WHITE) :
            (this->castling & castling::BLACK);

        if (castling_removed) {
            this->castling ^= castling_removed;
            this->hash ^= zobrist::get_castling(castling_removed);
        }
    }
    else if (piece_type == piece::type::ROOK) {
        // Removes castling right
        i8 castling_removed = castling::create(move_from) & this->castling;

        if (castling_removed) {
            this->castling ^= castling_removed;
            this->hash ^= zobrist::get_castling(castling_removed);
        }
    }
    else if (piece_type == piece::type::PAWN) {
        // Updates half move counter
        this->halfmove = 0;

        // Double push
        if (std::abs(move_from - move_to) == 16) {
            // Updates enpassant
            this->enpassant = move_to ^ 8;
            this->hash ^= zobrist::get_enpassant(square::get_file(this->enpassant));
        }
    }

    // Checks move type
    if (move_type == move::type::CASTLING) {
        assert(this->get_type_at(move_from) == piece::type::KING);
        assert(this->get_type_at(move_to) == piece::type::ROOK);

        bool castle_short = move_to > move_from;

        i8 king_to = castling::get_king_to(this->color, castle_short);
        i8 rook_to = castling::get_rook_to(this->color, castle_short);

        this->remove(piece::type::KING, this->color, move_from);
        this->remove(piece::type::ROOK, this->color, move_to);

        this->place(piece::type::KING, this->color, king_to);
        this->place(piece::type::ROOK, this->color, rook_to);

        i8 king = piece::create(piece::type::KING, this->color);
        i8 rook = piece::create(piece::type::ROOK, this->color);

        this->hash ^= zobrist::get_piece(king, move_from);
        this->hash ^= zobrist::get_piece(king, king_to);
        this->hash ^= zobrist::get_piece(rook, move_to);
        this->hash ^= zobrist::get_piece(rook, rook_to);

        this->hash_non_pawn[this->color] ^= zobrist::get_piece(king, move_from);
        this->hash_non_pawn[this->color] ^= zobrist::get_piece(king, king_to);
        this->hash_non_pawn[this->color] ^= zobrist::get_piece(rook, move_to);
        this->hash_non_pawn[this->color] ^= zobrist::get_piece(rook, rook_to);
    }
    else if (move_type == move::type::PROMOTION) {
        assert(piece_type == piece::type::PAWN);

        i8 promotion = move::get_promotion_type(move);

        this->remove(piece_type, this->color, move_from);
        this->place(promotion, this->color, move_to);

        this->hash ^= zobrist::get_piece(piece::create(piece_type, this->color), move_from);
        this->hash ^= zobrist::get_piece(piece::create(promotion, this->color), move_to);

        this->hash_pawn ^= zobrist::get_piece(piece::create(piece_type, this->color), move_from);
        this->hash_non_pawn[this->color] ^= zobrist::get_piece(piece::create(promotion, this->color), move_to);
    }
    else {
        assert(this->get_type_at(move_to) == piece::type::NONE);
        
        this->remove(piece_type, this->color, move_from);
        this->place(piece_type, this->color, move_to);

        i8 piece = piece::create(piece_type, this->color);

        this->hash ^= zobrist::get_piece(piece, move_from);
        this->hash ^= zobrist::get_piece(piece, move_to);

        if (piece_type == piece::type::PAWN) {
            this->hash_pawn ^= zobrist::get_piece(piece, move_from);
            this->hash_pawn ^= zobrist::get_piece(piece, move_to);
        }
        else {
            this->hash_non_pawn[this->color] ^= zobrist::get_piece(piece, move_from);
            this->hash_non_pawn[this->color] ^= zobrist::get_piece(piece, move_to);
        }
    }

    // Captures enpassant pawn
    if (move_type == move::type::ENPASSANT) {
        i8 enpassant_square = move_to ^ 8;

        assert(piece_type == piece::type::PAWN);
        assert(move_to == this->history.back().enpassant);

        this->remove(piece::type::PAWN, !this->color, enpassant_square);

        this->hash ^= zobrist::get_piece(piece::create(piece::type::PAWN, !this->color), enpassant_square);
        this->hash_pawn ^= zobrist::get_piece(piece::create(piece::type::PAWN, !this->color), enpassant_square);
    }

    // Updates color
    this->color = !this->color;
    this->hash ^= zobrist::get_color();

    // Update masks
    this->update_masks();

    // Checks hash
    assert(this->hash == this->get_hash_slow());
    assert(this->hash_pawn == this->get_hash_pawn_slow());
    assert(this->hash_non_pawn[0] == this->get_hash_non_pawn_slow(0));
    assert(this->hash_non_pawn[1] == this->get_hash_non_pawn_slow(1));
};

void Board::unmake(u16 move)
{
    // Reverts history
    auto undo = this->history.back();
    this->history.pop_back();

    this->hash = undo.hash;
    this->hash_pawn = undo.hash_pawn;
    this->hash_non_pawn[0] = undo.hash_non_pawn[0];
    this->hash_non_pawn[1] = undo.hash_non_pawn[1];
    this->castling = undo.castling;
    this->enpassant = undo.enpassant;
    this->halfmove = undo.halfmove;
    this->checkers = undo.checkers;
    this->blockers[0] = undo.blockers[0];
    this->blockers[1] = undo.blockers[1];

    this->color = !this->color;
    this->ply -= 1;

    // Gets move data
    i8 move_from = move::get_from(move);
    i8 move_to = move::get_to(move);
    u16 move_type = move::get_type(move);

    // Checks move type
    if (move_type == move::type::CASTLING) {
        bool castle_short = move_to > move_from;

        i8 king_to = castling::get_king_to(this->color, castle_short);
        i8 rook_to = castling::get_rook_to(this->color, castle_short);

        assert(this->get_type_at(king_to) == piece::type::KING);
        assert(this->get_type_at(rook_to) == piece::type::ROOK);

        this->remove(piece::type::KING, this->color, king_to);
        this->remove(piece::type::ROOK, this->color, rook_to);

        this->place(piece::type::KING, this->color, move_from);
        this->place(piece::type::ROOK, this->color, move_to);

        return;
    }
    else if (move_type == move::type::PROMOTION) {
        i8 promotion = move::get_promotion_type(move);

        assert(promotion == this->get_type_at(move_to));

        this->remove(promotion, this->color, move_to);
        this->place(piece::type::PAWN, this->color, move_from);

        if (undo.captured != piece::type::NONE) {
            this->place(undo.captured, !this->color, move_to);
        }

        return;
    }
    else {
        assert(this->get_type_at(move_to) != piece::type::NONE);
        assert(this->get_type_at(move_from) == piece::type::NONE);

        i8 piece_type = this->get_type_at(move_to);

        this->remove(piece_type, this->color, move_to);
        this->place(piece_type, this->color, move_from);
    }

    // Places captured piece
    if (move_type == move::type::ENPASSANT) {
        assert(this->get_type_at(this->enpassant ^ 8) == piece::type::NONE);

        this->place(piece::type::PAWN, !this->color, this->enpassant ^ 8);
    }
    else if (undo.captured != piece::type::NONE) {
        this->place(undo.captured, !this->color, move_to);
    }

    // Checks hash
    assert(this->hash == this->get_hash_slow());
    assert(this->hash_pawn == this->get_hash_pawn_slow());
    assert(this->hash_non_pawn[0] == this->get_hash_non_pawn_slow(0));
    assert(this->hash_non_pawn[1] == this->get_hash_non_pawn_slow(1));
};

void Board::make_null()
{
    // Saves info
    this->history.push_back(Undo {
        .hash = this->hash,
        .hash_pawn = this->hash_pawn,
        .hash_non_pawn = {
            this->hash_non_pawn[0],
            this->hash_non_pawn[1]
        },
        .castling = this->castling,
        .enpassant = this->enpassant,
        .captured = piece::type::NONE,
        .halfmove = this->halfmove,
        .checkers = this->checkers
    });

    // Updates color
    this->color = !this->color;
    this->hash ^= zobrist::get_color();

    // Updates enpassant square
    if (this->enpassant != square::NONE) {
        this->hash ^= zobrist::get_enpassant(square::get_file(this->enpassant));
        this->enpassant = square::NONE;
    }

    // Updates move counter
    this->ply += 1;

    // Updates masks
    this->checkers = 0ULL;
};

void Board::unmake_null()
{
    auto undo = this->history.back();
    this->history.pop_back();

    this->hash = undo.hash;
    this->hash_pawn = undo.hash_pawn;
    this->hash_non_pawn[0] = undo.hash_non_pawn[0];
    this->hash_non_pawn[1] = undo.hash_non_pawn[1];
    this->castling = undo.castling;
    this->enpassant = undo.enpassant;
    this->halfmove = undo.halfmove;
    this->checkers = undo.checkers;

    this->color = !this->color;
    this->ply -= 1;
};

void Board::remove(i8 type, i8 color, i8 square)
{
    assert(piece::type::is_valid(type));
    assert(color::is_valid(color));
    assert(square::is_valid(square));

    assert(this->get_type_at(square) == type);
    assert(this->get_color_at(square) == color);
    assert(this->get_piece_at(square) == piece::create(type, color));

    this->pieces[type] ^= 1ULL << square;
    this->colors[color] ^= 1ULL << square;
    this->board[square] = piece::NONE;
};

void Board::place(i8 type, i8 color, i8 square)
{
    assert(piece::type::is_valid(type));
    assert(color::is_valid(color));
    assert(square::is_valid(square));

    assert(this->get_type_at(square) == piece::type::NONE);
    assert(this->get_color_at(square) == color::NONE);
    assert(this->get_piece_at(square) == piece::NONE);

    this->pieces[type] |= 1ULL << square;
    this->colors[color] |= 1ULL << square;
    this->board[square] = piece::create(type, color);
};

void Board::update_masks()
{
    this->update_checkers();
    this->update_blockers<color::WHITE>();
    this->update_blockers<color::BLACK>();
};

void Board::print()
{
    for (i32 rank = 7; rank >= 0; --rank) {
        char line[] = ". . . . . . . .";

        for (i32 file = 0; file < 8; ++file) {
            i8 square = square::create(file, rank);

            if (this->board[square] == piece::NONE) {
                continue;
            }

            line[2 * file] = piece::get_char(this->board[square]);
        }

        printf("%s\n", line);
    }

    printf("\n");
};
