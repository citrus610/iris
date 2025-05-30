#include "history.h"

namespace history::quiet
{

i16& Table::get(Board& board, const u16& move)
{
    return this->data[board.get_color()][move::get_from(move)][move::get_to(move)];
};

void Table::update(Board& board, const u16& move, i16 bonus)
{
    history::update<history::quiet::MAX>(this->get(board, move), bonus);
};

};