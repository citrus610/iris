#pragma once

#define INCBIN_PREFIX
#define INCBIN_STYLE INCBIN_STYLE_SNAKE

#include "../chess/chess.h"
#include "../util/incbin.h"

#ifdef __AVX2__
    #include <x86intrin.h>
#endif

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

struct Parameters
{
    alignas(32) i16 in_weights[size::INPUT][size::HIDDEN];
    alignas(32) i16 in_biases[size::HIDDEN];
    alignas(32) i16 out_weights[2][size::HIDDEN];
    alignas(32) i16 out_bias;
};

inline Parameters PARAMS;

class Feature
{
public:
    i8 piece = piece::NONE;
    i8 square = square::NONE;
public:
    usize get_index(i8 color);
};

struct Update
{
    arrayvec<Feature, 2> adds = {};
    arrayvec<Feature, 2> subs = {};
};

class Accumulator
{
public:
    alignas(32) i16 data[2][size::HIDDEN];
    Update update;
public:
    void clear();
    void refresh(Board& board);
    void make(const Accumulator& parent, i8 color);
public:
    void edit_add1_sub1(const Accumulator& parent, usize add1, usize sub1, i8 color);
    void edit_add1_sub2(const Accumulator& parent, usize add1, usize sub1, usize sub2, i8 color);
    void edit_add2_sub2(const Accumulator& parent, usize add1, usize add2, usize sub1, usize sub2, i8 color);
};

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
    void refresh(Board& board);
    void make(Board& board, const u16& move);
    void unmake();
};

#ifdef __AVX2__
    template <usize SIZE, i32 SCALE>
    constexpr __m256i get_linear(i16 inputs[SIZE], i16 weights[SIZE])
    {
        auto vec = _mm256_setzero_si256();

        for (usize i = 0; i < SIZE; i += 16) {
            // Loads input vector
            auto input = _mm256_load_si256((const __m256i*)&inputs[i]);

            // Calculates clipped relu
            auto crelu = _mm256_min_epi16(_mm256_max_epi16(input, _mm256_setzero_si256()), _mm256_set1_epi16(i16(SCALE)));

            // Loads weight vector
            auto weight = _mm256_load_si256((const __m256i*)&weights[i]);

            // Calculates screlu * weight by doing (crelu * weight) * crelu
            auto product = _mm256_madd_epi16(_mm256_mullo_epi16(crelu, weight), crelu);

            // Adds the results
            vec = _mm256_add_epi32(vec, product);
        }

        return vec;
    };
#else
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
#endif

void init();

};