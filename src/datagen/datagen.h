#pragma once

#include <iomanip>
#include "game.h"

namespace datagen
{

#ifdef DATAGEN
    constexpr bool GENERATING = true;
#else
    constexpr bool GENERATING = false;
#endif

constexpr usize MAX_VISIT = 400;
constexpr usize MAX_OPENING = 8;
constexpr usize MAX_POSITION = 200000000;
constexpr usize SAVE_INTERVAL = 10;

constexpr i32 MAX_OPENING_DELTA = 400;

class Rng
{
public:
    u64 seed;
public:
    Rng(u64 id) {
        seed = timer::get_current() + id;
    };
public:
    u64 get() {
        this->seed ^= this->seed >> 12;
        this->seed ^= this->seed << 25;
        this->seed ^= this->seed >> 27;

        return this->seed * 2685821657736338717Ull;
    };
};

inline Board get_random_opening(Rng& rng)
{
    auto board = Board();

    for (usize i = 0; i < MAX_OPENING; ++i) {
        auto moves = move::gen::get_legal(board);

        if (moves.size() == 0) {
            return get_random_opening(rng);
        }

        board.make(moves[rng.get() % moves.size()]);
    }

    if (move::gen::get_legal(board).size() == 0) {
        return get_random_opening(rng);
    }

    return board;
};

inline i32 get_opening_score(const Board& board)
{
    auto engine = search::Engine();
    engine.set({ .hash = 8, .threads = 1 });

    return game::search(engine, board).score;
};

inline void run(u64 thread_count)
{
    // Stats
    std::atomic<u64> positions = 0;
    std::atomic<u64> win = 0;
    std::atomic<u64> loss = 0;
    std::atomic<u64> draw = 0;

    // Threads
    std::vector<std::thread> threads;
    std::mutex mtx;

    for (u64 i = 0; i < thread_count; ++i) {
        threads.emplace_back([&] (u64 id) {
            // Creates rng
            auto rng = Rng(id);
            
            // Loops
            u64 games = 0;
            std::vector<std::string> lines;

            while (true) {
                // Gets a random opening
                auto board = get_random_opening(rng);

                // Removes super unbalanced openings
                if (std::abs(get_opening_score(board)) >= MAX_OPENING_DELTA) {
                    continue;
                }

                // Plays a game
                auto game_result = game::run(board);
                
                // Stores results
                usize visited = 0;

                for (auto& p : game_result.positions) {
                    if (visited >= MAX_VISIT) {
                        break;
                    }

                    std::string wdl_str =
                        game_result.wdl > 0.9f ? "1.0" :
                        game_result.wdl < 0.1f ? "0.0" :
                        "0.5";

                    lines.push_back(p.fen + " | " + std::to_string(p.score) + " | " + wdl_str);

                    visited += 1;
                }

                // Updates stats
                positions += game_result.positions.size();

                if (game_result.wdl > 0.9f) {
                    win += 1;
                }
                else if (game_result.wdl < 0.1f) {
                    loss += 1;
                }
                else {
                    draw += 1;
                }

                games += 1;

                // Only save now and then
                if (games % 100 == 0) {
                    std::lock_guard<std::mutex> lk(mtx);

                    std::ofstream o("out.txt", std::ios::out | std::ios::app);

                    for (auto& line : lines) {
                        o << line << std::endl;
                    }

                    lines.clear();
                    o.close();

                    // Reports
                    f32 win_ratio = (f32(win.load()) + f32(draw.load()) / 2.0f) / f32(win.load() + draw.load() + loss.load());
                    std::cout <<
                        "\rprogress: " << (f64(positions) / f64(MAX_POSITION) * 100.0) <<
                        " | positions: " << positions << "/" << MAX_POSITION <<
                        " | win ratio: " << win_ratio << "                        ";
                }

                // Max positions reached
                if (positions >= MAX_POSITION) {
                    break;
                }
            }
        }, i);
    }

    for (auto& t : threads) {
        t.join();
    }
};

};