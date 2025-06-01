#include "history.h"

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