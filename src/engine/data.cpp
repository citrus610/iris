#include "data.h"

Data::Data(const Board& board)
{
    this->board = board;
    this->clear();
};

void Data::clear()
{
    this->ply = 0;
    this->stack.clear();
    this->nodes = 0;
    this->seldepth = 0;
    this->counter = node::Counter();
};

void Data::make(const u16& move)
{
    this->stack[this->ply].move = move;
    this->stack[this->ply].conthist = &this->history.cont.get_entry(this->board, move);

    this->board.make(move);
    this->ply += 1;
};

void Data::unmake(const u16& move)
{
    this->board.unmake(move);
    this->ply -= 1;
};

void Data::make_null()
{
    this->stack[this->ply].move = move::NONE;
    this->stack[this->ply].conthist = nullptr;

    this->board.make_null();
    this->ply += 1;
};

void Data::unmake_null()
{
    this->board.unmake_null();
    this->ply -= 1;
};