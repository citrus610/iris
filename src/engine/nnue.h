#pragma once

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "../chess/chess.h"
#include "../util/incbin.h"

namespace nnue
{

namespace size
{
    constexpr usize INPUT = 768;
    constexpr usize HIDDEN = 128;
};

namespace scale
{
    constexpr i32 EVAL = 400;
    constexpr i32 L0 = 255;
    constexpr i32 L1 = 64;
};

struct Accumulator
{
    i16 data[2][size::HIDDEN];
};

struct Parameters
{
    i16 in_weights[size::INPUT][size::HIDDEN];
    i16 in_biases[size::HIDDEN];
    i16 out_weights[2][size::HIDDEN];
    i16 out_bias;
};

inline Parameters PARAMS;

class Net
{
private:
    Accumulator stack[MAX_PLY + 8];
    usize index;
public:
    Net();
public:
    i32 get_eval(i8 color);
public:
    template <bool ADD> void update(i8 color, i8 type, i8 square);
    void clear();
    void refresh(Board& board);
    void make(Board& board, const u16& move);
    void unmake();
};

constexpr std::pair<usize, usize> get_index(i8 color, i8 type, i8 square)
{
    return {
        usize(384) * usize(color) + usize(64) * usize(type) + usize(square),
        usize(384) * usize(!color) + usize(64) * usize(type) + usize(square ^ 56)
    };
};

template <i32 SCALE>
constexpr i32 get_screlu(i32 input)
{
    i32 value = std::clamp(input, 0, SCALE);

    return value * value;
};

template <usize SIZE, i32 SCALE>
constexpr i32 get_linear(i16 inputs[SIZE], i16 weights[SIZE])
{
    i32 value = 0;

    for (usize i = 0; i < SIZE; ++i) {
        value += get_screlu<SCALE>(inputs[i]) * weights[i];
    }

    return value;
};

void init();

};