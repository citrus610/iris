#include "search.h"

namespace search
{

Engine::Engine()
{
    this->clear();
};

void Engine::clear()
{
    this->running.clear();
    this->thread = nullptr;
    this->timer.clear();
    this->table.clear();
};

void Engine::set(uci::parse::Setoption uci_setoption)
{
    this->table.init(uci_setoption.hash);
};

bool Engine::search(Board uci_board, uci::parse::Go uci_go)
{
    if (this->running.test() || this->thread != nullptr) {
        return false;
    }

    // Updates data
    this->table.update();
    this->timer.set(uci_go, uci_board.get_color());

    // Starts the search thread
    this->running.test_and_set();

    this->thread = new std::thread([&] (Board board, uci::parse::Go go) {
        // Inits search data
        auto data = Data(board);

        // Storing best pv lines found in each iteration
        std::vector<pv::Line> pv_history = {};

        // Preivous search score
        i32 score_old = -eval::score::INFINITE;

        // Iterative deepening
        for (i32 i = 1; i < go.depth; ++i) {
            // Clear search data
            data.clear();

            // Principle variation search
            u64 time_1 = timer::get_current();
            i32 score = this->pvsearch<true>(data, -eval::score::INFINITE, eval::score::INFINITE, i);
            u64 time_2 = timer::get_current();

            // Avoids returning false score when stopped early
            if (!this->running.test()) {
                score = score_old;
            }
            
            // Updates score
            score_old = score;

            // Saves pv line
            if (data.stack[0].pv.count != 0 && data.stack[0].pv[0] != move::NONE) {
                pv_history.push_back(data.stack[0].pv);
            }

            // Prints infos
            u64 time = time_2 - time_1;

            uci::print::info(i, data.seldepth, score, data.nodes, time, 0, pv_history.back());

            // Avoids searching too shallow
            if (i < 4) {
                continue;
            }

            // Checks time
            if (!go.infinite && this->timer.is_over_soft()) {
                this->running.clear();
            }

            if (!this->running.test()) {
                break;
            }
        }

        // Prints best move
        uci::print::best(pv_history.back()[0]);
    }, uci_board, uci_go);

    return true;
};

bool Engine::stop()
{
    if (this->thread == nullptr) {
        return false;
    }

    this->running.clear();

    return this->join();
};

bool Engine::join()
{
    if (this->thread == nullptr) {
        return false;
    }

    if (!this->thread->joinable()) {
        return false;
    }

    this->thread->join();

    delete this->thread;
    this->thread = nullptr;

    return true;
};

template <bool PV>
i32 Engine::pvsearch(Data& data, i32 alpha, i32 beta, i32 depth)
{
    // Gets is root node
    const bool is_root = data.ply == 0;

    // Quiensence search
    if (depth <= 0) {
        return this->qsearch<PV>(data, alpha, beta);
    }

    // Aborts search
    if ((data.nodes & 0xFFF) == 0) {
        if (this->timer.is_over_hard()) {
            this->running.clear();
        }
    }

    if (!this->running.test()) {
        return eval::score::DRAW;
    }

    // Updates data
    data.stack[data.ply].pv.count = 0;
    data.nodes += 1;
    data.seldepth = std::max(data.seldepth, data.ply);

    // Early stop conditions
    if (!is_root) {
        // Checks draw
        if (data.board.is_draw()) {
            return eval::score::DRAW;
        }

        // Mate distance pruning
        alpha = std::max(alpha, data.ply - eval::score::MATE);
        beta = std::min(beta, eval::score::MATE - data.ply - 1);

        if (alpha >= beta) {
            return alpha;
        }
    }

    // In check
    const bool is_in_check = data.board.get_checkers();

    // Resets killer move
    data.stack[data.ply + 1].killer = move::NONE;

    // Best score
    i32 best = -eval::score::INFINITE;

    // Generates moves
    auto picker = order::Picker(data, move::NONE);
    auto legals = 0;

    // Iterates moves
    while (true)
    {
        // Gets move
        const u16 move = picker.get(data);

        if (!move) {
            break;
        }

        // Checks legality
        if (!data.board.is_legal(move)) {
            continue;
        }

        legals += 1;

        // Checks for quiet
        bool is_quiet = data.board.is_quiet(move);

        // Makes move
        data.make(move);

        // Searches
        i32 score = -eval::score::INFINITE;
        i32 depth_next = depth - 1;

        // Scouts with null window for non pv nodes
        if (!PV || legals > 1) {
            score = -this->pvsearch<false>(data, -alpha - 1, -alpha, depth_next);
        }

        // Searches as pv node for first child or researches after scouting
        if (PV && (legals == 1 || score > alpha)) {
            score = -this->pvsearch<true>(data, -beta, -alpha, depth_next);
        }

        // Unmakes move
        data.unmake(move);

        // Aborts search
        if (!this->running.test()) {
            return eval::score::DRAW;
        }

        // Updates best
        if (score > best) {
            best = score;
    
            // Updates alpha
            if (score > alpha) {
                alpha = score;
    
                // Updates pv line
                data.stack[data.ply].pv.update(move, data.stack[data.ply + 1].pv);
            }
        }

        // Cutoff
        if (score >= beta) {
            if (is_quiet) {
                // Stores killer moves
                data.stack[data.ply].killer = move;
            }

            break;
        }
    }

    // Checks stalemate or checkmate
    if (legals == 0) {
        if (is_in_check) {
            return -eval::score::MATE + data.ply;
        }
        else {
            return eval::score::DRAW;
        }
    }

    return best;
};

template <bool PV>
i32 Engine::qsearch(Data& data, i32 alpha, i32 beta)
{
    // Aborts search
    if ((data.nodes & 0xFFF) == 0) {
        if (this->timer.is_over_hard()) {
            this->running.clear();
        }
    }

    if (!this->running.test()) {
        return eval::score::DRAW;
    }

    // Updates data
    data.stack[data.ply].pv.count = 0;
    data.nodes += 1;
    data.seldepth = std::max(data.seldepth, data.ply);

    // Checks draw
    if (data.board.is_draw()) {
        return eval::score::DRAW;
    }

    // In check
    const bool is_in_check = data.board.get_checkers();

    // Max ply reached
    if (data.ply >= MAX_PLY) {
        return is_in_check ? eval::score::DRAW : eval::get(data.board);
    }

    // Gets static eval
    i32 eval = eval::score::NONE;

    if (!is_in_check) {
        eval = eval::get(data.board);
    }

    // Best score
    i32 best = -eval::score::INFINITE;

    if (!is_in_check) {
        best = eval;
    
        // Prunes
        if (eval >= beta) {
            return eval;
        }
    
        // Updates alpha
        if (alpha < eval) {
            alpha = eval;
        }
    }

    // Generates moves
    auto picker = order::Picker(data, move::NONE, !is_in_check);
    auto legals = 0;

    // Iterates moves
    while (true)
    {
        // Gets move
        const u16 move = picker.get(data);

        if (!move) {
            break;
        }

        // Checks legality
        if (!data.board.is_legal(move)) {
            continue;
        }

        legals += 1;

        // Makes move
        data.make(move);

        // Searches
        i32 score = -this->qsearch<PV>(data, -beta, -alpha);

        // Unmakes move
        data.unmake(move);

        // Updates best
        if (score > best) {
            best = score;

            // Updates alpha
            if (score > alpha) {
                alpha = score;
    
                // Updates pv line
                data.stack[data.ply].pv.update(move, data.stack[data.ply + 1].pv);
            }
        }

        // Cutoff
        if (score >= beta) {
            break;
        }
    };

    // Returns mate score
    if (legals == 0 && is_in_check) {
        return -eval::score::MATE + data.ply;
    }

    return best;
};

template i32 Engine::pvsearch<true>(Data&, i32, i32, i32);
template i32 Engine::pvsearch<false>(Data&, i32, i32, i32);

template i32 Engine::qsearch<true>(Data&, i32, i32);
template i32 Engine::qsearch<false>(Data&, i32, i32);

};