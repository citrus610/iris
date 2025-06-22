#include "nnue.h"

namespace nnue
{

Net::Net()
{
    for (i8 color = 0; color < 2; ++color) {
        for (usize i = 0; i < size::HIDDEN; ++i) {
            this->accumulator.data[color][i] = PARAMS.in_biases[i];
        }
    }

    this->stack.reserve(512);
    this->stack.clear();
};

i32 Net::get_eval(i8 color)
{
    i32 score = 0;

    score += get_linear<size::HIDDEN, scale::L0>(this->accumulator.data[color], PARAMS.out_weights[0]);
    score += get_linear<size::HIDDEN, scale::L0>(this->accumulator.data[!color], PARAMS.out_weights[1]);

    return (score / scale::L0 + PARAMS.out_bias) * scale::EVAL / (scale::L0 * scale::L1);
};

template <bool ADD>
void Net::update(i8 color, i8 type, i8 square)
{
    const auto [index_white, index_black] = get_index(color, type, square);

    for (usize i = 0; i < size::HIDDEN; ++i) {
        if constexpr (ADD) {
            this->accumulator.data[0][i] += PARAMS.in_weights[index_white][i];
            this->accumulator.data[1][i] += PARAMS.in_weights[index_black][i];
        }
        else {
            this->accumulator.data[0][i] -= PARAMS.in_weights[index_white][i];
            this->accumulator.data[1][i] -= PARAMS.in_weights[index_black][i];
        }
    }
};

void Net::add()
{
    this->stack.push_back(this->accumulator);
};

void Net::pop()
{
    assert(!this->stack.empty());

    this->accumulator = this->stack.back();
    this->stack.pop_back();
};

};