#include "order.h"

namespace order
{

Picker::Picker(Data& data, u16 hasher, bool skip)
{
    this->moves.clear();
    this->hasher = hasher;
    this->killer = data.stack[data.ply].killer;
    this->index = 0;
    this->stage = hasher != move::NONE ? Stage::HASHER : Stage::NOISY_GEN;
    this->skip = skip;
};

u16 Picker::get(Data& data)
{
    if (this->stage == Stage::HASHER) {
        this->stage = Stage::NOISY_GEN;

        if (data.board.is_pseudo_legal(this->hasher) && !(this->skip && data.board.is_quiet(this->hasher))) {
            return this->hasher;
        }
    }

    if (this->stage == Stage::NOISY_GEN) {
        this->stage = Stage::NOISY;
        this->index = 0;
        this->moves = move::gen::get<move::gen::type::NOISY>(data.board);
        this->score_noisy(data);
    }

    if (this->stage == Stage::NOISY) {
        while (this->index < this->moves.size())
        {
            this->sort();
            auto best = this->moves[this->index];
            this->index += 1;

            if (best == this->hasher) {
                continue;
            }

            return best;
        }

        this->stage = Stage::KILLER;
    }

    if (skip) {
        return move::NONE;
    }

    if (this->stage == Stage::KILLER) {
        this->stage = Stage::QUIET_GEN;

        if (this->killer != this->hasher && data.board.is_quiet(this->killer) && data.board.is_pseudo_legal(this->killer)) {
            return this->killer;
        }
    }

    if (this->stage == Stage::QUIET_GEN) {
        this->stage = Stage::QUIET;
        this->index = 0;
        this->moves = move::gen::get<move::gen::type::QUIET>(data.board);
        this->score_quiet(data);
    }

    if (this->stage == Stage::QUIET) {
        while (this->index < this->moves.size())
        {
            this->sort();
            auto best = this->moves[this->index];
            this->index += 1;

            if (best == this->hasher || best == this->killer) {
                continue;
            }

            return best;
        }
    }

    return move::NONE;
};

Stage Picker::get_stage()
{
    return this->stage;
};

bool Picker::is_skipped()
{
    return skip;
};

void Picker::sort()
{
    usize best = this->index;

    for (usize i = this->index + 1; i < this->moves.size(); ++i) {
        if (this->scores[i] > this->scores[best]) {
            best = i;
        }
    }

    std::swap(this->moves[best], this->moves[this->index]);
    std::swap(this->scores[best], this->scores[this->index]);
};

void Picker::score_quiet(Data& data)
{
    for (usize i = 0; i < this->moves.size(); ++i) {
        this->scores[i] = data.history.quiet.get(data.board, this->moves[i]);
    }
};

void Picker::score_noisy(Data& data)
{
    for (usize i = 0; i < this->moves.size(); ++i) {
        const i8 captured = data.board.get_captured_type(this->moves[i]);

        this->scores[i] = eval::PIECE_VALUE[captured] * 16 + data.history.noisy.get(data.board, this->moves[i], captured);
    }
};

void Picker::skip_quiets()
{
    this->skip = true;
};

};