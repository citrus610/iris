#include "history.h"
#include "data.h"

namespace history::quiet
{

i16& Table::get(Board& board, const u16& move)
{
    assert(square::is_valid(move::get_from(move)));
    assert(square::is_valid(move::get_to(move)));

    return this->data[board.get_color()][move::get_from(move)][move::get_to(move)];
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

i16 Table::get(Data& data, const u16& move)
{
    return this->get(data, move, 1);
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
};

void Table::update(Data& data, const u16& move, i32 offset, i16 bonus)
{
    if (data.ply < offset || data.stack[data.ply - offset].conthist == nullptr) {
        return;
    }

    data.stack[data.ply - offset].conthist->update(data.board, move, bonus);
};

};

namespace history
{

i32 Table::get_score_quiet(Data& data, const u16& move)
{
    return this->quiet.get(data.board, move) + this->cont.get(data, move);
};

i32 Table::get_score_noisy(Data& data, const u16& move)
{
    return this->noisy.get(data.board, move);
};

};