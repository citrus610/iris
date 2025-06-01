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
    constexpr i32 MVV_LVA[5][6] = {
        { 150, 140, 130, 120, 110, 100 },
        { 250, 240, 230, 220, 210, 200 },
        { 350, 340, 330, 320, 210, 300 },
        { 450, 440, 430, 420, 410, 400 },
        { 550, 540, 530, 520, 510, 500 }
    };

    for (usize i = 0; i < this->moves.size(); ++i) {
        const u16& move = this->moves[i];

        i8 piece = data.board.get_type_at(move::get_from(move));
        i8 captured = move::get_type(move) == move::type::ENPASSANT ? piece::type::PAWN : data.board.get_type_at(move::get_to(move));

        if (captured == piece::type::NONE) {
            captured = piece::type::PAWN;
        }

        this->scores[i] = MVV_LVA[captured][piece];
    }
};

void Picker::skip_quiets()
{
    this->skip = true;
};

};