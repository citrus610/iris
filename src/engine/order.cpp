#include "order.h"

namespace order
{

Picker::Picker(Data& data, u16 hasher, bool skip)
{
    this->moves.clear();
    this->baddies.clear();
    this->hasher = hasher;
    this->killer = data.stack[data.ply].killer;
    this->stage = hasher != move::NONE ? Stage::HASHER : Stage::NOISY_GEN;
    this->index = 0;
    this->index_bad = 0;
    this->skip = skip;
};

u16 Picker::get(Data& data)
{
    // Hash move
    if (this->stage == Stage::HASHER) {
        this->stage = Stage::NOISY_GEN;

        if (data.board.is_pseudo_legal(this->hasher) && !(this->skip && data.board.is_quiet(this->hasher))) {
            return this->hasher;
        }
    }

    // Generates noisy moves
    if (this->stage == Stage::NOISY_GEN) {
        this->stage = Stage::NOISY_GOOD;
        this->index = 0;
        this->moves = move::gen::get<move::gen::type::NOISY>(data.board);
        this->score_noisy(data);
    }

    // Returns good noisy move
    if (this->stage == Stage::NOISY_GOOD) {
        while (this->index < this->moves.size())
        {
            this->sort();

            auto best = this->moves[this->index];
            auto score = this->scores[this->index];

            this->index += 1;

            if (best == this->hasher) {
                continue;
            }

            // Adds moves that fail to pass SEE to the baddies list :D
            if (!see::is_ok(data.board, best, -score / 32)) {
                this->baddies.add(best);
                continue;
            }

            return best;
        }

        this->stage = Stage::KILLER;
    }

    // If skipped quiet moves, returns bad noisy moves
    if (skip) {
        this->stage = Stage::NOISY_BAD;
        goto noisy_baddies;
    }

    // Returns killer move
    if (this->stage == Stage::KILLER) {
        this->stage = Stage::QUIET_GEN;

        if (this->killer != this->hasher && data.board.is_quiet(this->killer) && data.board.is_pseudo_legal(this->killer)) {
            return this->killer;
        }
    }

    // Generates quiet moves
    if (this->stage == Stage::QUIET_GEN) {
        this->stage = Stage::QUIET;
        this->index = 0;
        this->moves = move::gen::get<move::gen::type::QUIET>(data.board);
        this->score_quiet(data);
    }

    // Returns quiet moves
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

        this->stage = Stage::NOISY_BAD;
    }

    // Returns bad noisy moves
    noisy_baddies:

    while (this->index_bad < this->baddies.size())
    {
        auto best = this->baddies[this->index_bad];

        this->index_bad += 1;

        if (best == this->hasher) {
            continue;
        }

        return best;
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
        this->scores[i] =
            data.history.quiet.get(data.board.get_color(), data.board.get_threats(), this->moves[i]) +
            data.history.pawn.get(data.board, this->moves[i]) +
            data.history.cont.get(data, this->moves[i], 1) +
            data.history.cont.get(data, this->moves[i], 2) +
            data.history.cont.get(data, this->moves[i], 4);
    }
};

void Picker::score_noisy(Data& data)
{
    for (usize i = 0; i < this->moves.size(); ++i) {
        const auto captured = data.board.get_captured_type(this->moves[i]);

        this->scores[i] =
            eval::PIECE_VALUE[captured] * 16 +
            data.history.noisy.get(data.board, this->moves[i], captured);
    }
};

void Picker::skip_quiets()
{
    this->skip = true;
};

};