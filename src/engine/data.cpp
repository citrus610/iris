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
};

void Data::make(const u16& move)
{
    this->stack[this->ply].move = move;

    this->board.make(move);
    this->ply += 1;
};

void Data::unmake(const u16& move)
{
    this->board.unmake(move);
    this->ply -= 1;

    this->stack[this->ply].move = move::NONE;
};

void Data::make_null()
{
    this->stack[this->ply].move = move::NONE;

    this->board.make(move::NONE);
    this->ply += 1;
};

void Data::unmake_null()
{
    this->board.unmake_null();
    this->ply -= 1;
};