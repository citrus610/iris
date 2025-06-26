#pragma once

#include "../engine/search.h"

namespace datagen::game
{

constexpr i32 MAX_DEPTH = 8;
constexpr i32 MAX_SCORE = 2000;

struct SearchResult
{
    i32 score = -eval::score::INFINITE;
    u16 move = move::NONE;
};

struct Position
{
    std::string fen;
    i32 score;
};

struct Result
{
    std::vector<Position> positions;
    f32 wdl;
};

inline SearchResult search(search::Engine& engine, const Board& board)
{
    auto result = SearchResult();

    // Updates engine
    engine.table.update();
    engine.timer.clear();
    engine.running.test_and_set();

    // Inits data
    auto data = new Data(board);

    // Search
    for (i32 i = 1; i <= MAX_DEPTH; ++i) {
        data->clear();

        result.score = engine.aspiration_window(*data, i, result.score);
        result.move = data->stack[0].pv[0];
    }

    // Clears data
    delete data;

    return result;
};

inline Result run(Board& board)
{
    auto result = Result();

    // Inits engines
    search::Engine engines[2] = { search::Engine(), search::Engine() };

    engines[0].set({ .hash = 8, .threads = 1 });
    engines[1].set({ .hash = 8, .threads = 1 });

    // Plays
    while (true)
    {
        // Search
        auto search_result = search(engines[board.get_color()], board);

        // Updates score to white relative
        search_result.score = board.get_color() == color::WHITE ? search_result.score : -search_result.score;

        // If the score is high enough, stop
        if (std::abs(search_result.score) >= MAX_SCORE) {
            result.wdl = search_result.score > 0 ? 1.0f : 0.0f;
            break;
        }

        // Adds position
        result.positions.push_back(Position {
            .fen = board.get_fen(),
            .score = search_result.score
        });

        // Updates board
        board.make(search_result.move);

        // Checks draw
        if (board.is_draw() || move::gen::get_legal(board).size() == 0) {
            result.wdl = 0.5f;
            break;
        }
    }

    return result;
};

};