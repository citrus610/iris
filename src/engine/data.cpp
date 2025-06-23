#include "data.h"

Data::Data(const Board& board, u64 id)
{
    this->board = board;
    this->id = id;
    this->clear();
};

void Data::clear()
{
    this->ply = 0;
    this->stack.clear();
    this->nnue.refresh(this->board);
    this->nodes = 0;
    this->seldepth = 0;
    this->counter = node::Counter();
};

void Data::make(const u16& move)
{
    this->stack[this->ply].move = move;
    this->stack[this->ply].conthist = &this->history.cont.get_entry(this->board, move);

    this->nnue.make(this->board, move);
    this->board.make(move);
    this->ply += 1;
};

void Data::unmake(const u16& move)
{
    this->nnue.unmake();
    this->board.unmake(move);
    this->ply -= 1;
};

void Data::make_null()
{
    this->stack[this->ply].move = move::NONE;
    this->stack[this->ply].conthist = nullptr;

    this->nnue.make(this->board, move::NONE);
    this->board.make_null();
    this->ply += 1;
};

void Data::unmake_null()
{
    this->nnue.unmake();
    this->board.unmake_null();
    this->ply -= 1;
};