#include "history.h"
#include "data.h"

namespace history::quiet
{

i16& Table::get(Board& board, const u16& move)
{
    assert(square::is_valid(move::get_from(move)));
    assert(square::is_valid(move::get_to(move)));

    const bool is_threaten_from = board.get_threats() & bitboard::create(move::get_from(move));
    const bool is_threaten_to = board.get_threats() & bitboard::create(move::get_to(move));

    return this->data[board.get_color()][move & 0xFFF][is_threaten_from][is_threaten_to];
};

void Table::update(Board& board, const u16& move, i16 bonus)
{
    history::update<history::quiet::MAX>(this->get(board, move), bonus);
};

};

namespace history::noisy
{

i16& Table::get(Board& board, const u16& move)
{
    assert(!board.is_quiet(move));

    return this->get(board, move, board.get_captured_type(move));
};

i16& Table::get(Board& board, const u16& move, i8 captured)
{
    assert(!board.is_quiet(move));

    const i8 to = move::get_to(move);
    const i8 piece = board.get_piece_at(move::get_from(move));
    
    assert(piece != piece::NONE);

    return this->data[piece][to][captured];
};

void Table::update(Board& board, const u16& move, i16 bonus)
{
    history::update<history::noisy::MAX>(this->get(board, move), bonus);
};

};

namespace history::cont
{

i16& Entry::get(Board& board, const u16& move)
{
    assert(board.is_quiet(move));

    const i8 to = move::get_to(move);
    const i8 piece = board.get_piece_at(move::get_from(move));

    return this->data[piece][to];
};

void Entry::update(Board& board, const u16& move, i16 bonus)
{
    history::update<history::cont::MAX>(this->get(board, move), bonus);
};

Entry& Table::get_entry(Board& board, const u16& move)
{
    const i8 to = move::get_to(move);
    const i8 piece = board.get_piece_at(move::get_from(move));

    return this->data[piece][to];
};

i16 Table::get(Data& data, const u16& move, i32 offset)
{
    if (data.ply < offset || data.stack[data.ply - offset].conthist == nullptr) {
        return 0;
    }

    return data.stack[data.ply - offset].conthist->get(data.board, move);
};

void Table::update(Data& data, const u16& move, i16 bonus)
{
    this->update(data, move, 1, bonus);
    this->update(data, move, 2, bonus);
    this->update(data, move, 4, bonus);
};

void Table::update(Data& data, const u16& move, i32 offset, i16 bonus)
{
    if (data.ply < offset || data.stack[data.ply - offset].conthist == nullptr) {
        return;
    }

    data.stack[data.ply - offset].conthist->update(data.board, move, bonus);
};

};

namespace history::corr
{

i16& Table::get(const i8 color, const u64& hash)
{
    assert(color::is_valid(color));
    assert((hash & MASK) < SIZE);

    return this->data[color][hash & MASK];
};

void Table::update(const i8 color, const u64& hash, i16 bonus)
{
    history::update<history::corr::MAX>(this->get(color, hash), bonus);
};

};

namespace history
{

i32 Table::get_correction(Board& board)
{
    i32 correction = 0;

    correction += i32(this->corr_pawn.get(board.get_color(), board.get_hash_pawn())) * tune::CORR_WEIGHT_PAWN;
    correction += i32(this->corr_non_pawn[color::WHITE].get(board.get_color(), board.get_hash_non_pawn(color::WHITE))) * tune::CORR_WEIGHT_NON_PAWN;
    correction += i32(this->corr_non_pawn[color::BLACK].get(board.get_color(), board.get_hash_non_pawn(color::BLACK))) * tune::CORR_WEIGHT_NON_PAWN;

    return correction / history::corr::SCALE;
};

void Table::update_correction(Board& board, i16 bonus)
{
    this->corr_pawn.update(board.get_color(), board.get_hash_pawn(), bonus);
    this->corr_non_pawn[color::WHITE].update(board.get_color(), board.get_hash_non_pawn(color::WHITE), bonus);
    this->corr_non_pawn[color::BLACK].update(board.get_color(), board.get_hash_non_pawn(color::BLACK), bonus);
};

};