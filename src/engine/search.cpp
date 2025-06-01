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
            i32 score = this->aspiration_window(data, i, score_old);
            u64 time_2 = timer::get_current();

            // Avoids returning false score when stopping early
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
            uci::print::info(
                i,
                data.seldepth,
                score,
                data.nodes,
                time_2 - time_1,
                this->table.hashfull(),
                pv_history.back()
            );

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

i32 Engine::aspiration_window(Data& data, i32 depth, i32 score_old)
{
    i32 score = -eval::score::INFINITE;
    i32 delta = tune::aw::DELTA;

    // Sets the window
    i32 alpha = -eval::score::INFINITE;
    i32 beta = eval::score::INFINITE;

    if (depth >= tune::aw::DEPTH) {
        alpha = std::max(score_old - delta, -eval::score::INFINITE);
        beta = std::min(score_old + delta, eval::score::INFINITE);
    }

    // Loops
    while (true)
    {
        // Principle variation search
        score = this->pvsearch<true>(data, alpha, beta, depth);

        // Aborts
        if (!this->running.test()) {
            break;
        }

        // Updates window
        if (score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(score - delta, -eval::score::INFINITE);
        }
        else if (score >= beta) {
            beta = std::min(score + delta, eval::score::INFINITE);
        }
        else {
            break;
        }

        // Increase delta
        delta = delta + delta / 2;
    }

    return score;
};

template <bool PV>
i32 Engine::pvsearch(Data& data, i32 alpha, i32 beta, i32 depth)
{
    // Gets is root node
    const bool is_root = data.ply == 0;

    if (!data.board.get_pieces(piece::type::KING, data.board.get_color())) {
        data.board.print();
    }

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

    // Probes transposition table
    auto [table_hit, table_entry] = this->table.get(data.board.get_hash());

    auto table_move = move::NONE;
    auto table_eval = eval::score::NONE;
    auto table_score = eval::score::NONE;
    auto table_depth = 0;
    auto table_bound = transposition::bound::NONE;
    auto table_pv = PV;

    if (table_hit) {
        table_move = table_entry->get_move();
        table_eval = table_entry->get_eval();
        table_score = table_entry->get_score(data.ply);
        table_depth = table_entry->get_depth();
        table_bound = table_entry->get_bound();
        table_pv |= table_entry->is_pv();

        // Cut off
        if (!PV && table_score != eval::score::NONE && table_depth >= depth) {
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= beta) ||
                (table_bound == transposition::bound::UPPER && table_score <= alpha)) {
                return table_score;
            }
        }
    }

    // Gets important values for search, prunings, extensions
    // In check
    const bool is_in_check = data.board.get_checkers();

    // Resets killer move
    data.stack[data.ply + 1].killer = move::NONE;

    // Static eval
    i32 eval = eval::score::NONE;
    i32 eval_static = eval::score::NONE;

    if (is_in_check) {
        data.stack[data.ply].eval = eval::score::NONE;
        goto loop;
    }
    else if (table_hit) {
        eval_static = table_eval != eval::score::NONE ? table_eval : eval::get(data.board);
        eval = eval_static;

        // Uses the node's score as a more accurate eval value
        if ((table_bound == transposition::bound::EXACT) ||
            (table_bound == transposition::bound::LOWER && table_score >= beta) ||
            (table_bound == transposition::bound::UPPER && table_score <= alpha)) {
            eval = table_score;
        }
    }
    else {
        eval_static = eval::get(data.board);
        eval = eval_static;

        // Stores this eval into the table
        table_entry->set(
            data.board.get_hash(),
            move::NONE,
            eval::score::NONE,
            eval_static,
            depth,
            this->table.age,
            table_pv,
            transposition::bound::NONE,
            data.ply
        );
    }

    data.stack[data.ply].eval = eval_static;

    // Reverse futility pruning
    if (!PV &&
        depth <= tune::rfp::DEPTH &&
        eval < eval::score::MATE_FOUND &&
        eval >= beta + depth * tune::rfp::COEF) {
        return eval;
    }

    // Null move pruning
    if (!PV &&
        data.stack[data.ply - 1].move != move::NONE &&
        eval >= beta &&
        depth >= tune::nmp::DEPTH &&
        data.board.has_non_pawn(data.board.get_color())) {
        // Calculates reduction count based on depth and eval
        i32 reduction =
            tune::nmp::REDUCTION +
            depth / tune::nmp::DIVISOR_DEPTH +
            std::min((eval - beta) / tune::nmp::DIVISOR_EVAL, tune::nmp::REDUCTION_EVAL_MAX);
        
        // Makes null move
        data.make_null();

        // Scouts
        i32 score = -this->pvsearch<false>(data, -beta, -beta + 1, depth - reduction);

        // Unmakes
        data.unmake_null();

        // Returns score if fail high, we don't return false mate score
        if (score >= beta) {
            return score < eval::score::MATE_FOUND ? score : beta;
        }
    }

    // Move loop
    loop:

    // Best score
    i32 best = -eval::score::INFINITE;
    u16 best_move = move::NONE;
    i32 alpha_old = alpha;

    // Generates moves
    auto picker = order::Picker(data, table_move);
    auto legals = 0;
    auto quiets = arrayvec<u16, move::MAX>();

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

        if (legals > 1 + is_root * 2 &&
            depth >= tune::lmr::DEPTH &&
            is_quiet) {
            // Gets reduction count
            i32 reduction = tune::lmr::TABLE[depth][legals];

            // Clamps depth to avoid qsearch
            i32 depth_reduced = std::clamp(depth_next - reduction, 1, depth_next + 1);

            // Scouts
            score = -this->pvsearch<false>(data, -alpha - 1, -alpha, depth_reduced);

            // Failed high
            if (score > alpha && depth_reduced < depth_next) {
                score = -this->pvsearch<false>(data, -alpha - 1, -alpha, depth_next);
            }
        }
        // Scouts with null window for non pv nodes
        else if (!PV || legals > 1) {
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
            best_move = move;
    
            // Updates alpha
            if (score > alpha) {
                alpha = score;
    
                // Updates pv line
                data.stack[data.ply].pv.update(move, data.stack[data.ply + 1].pv);
            }
        }

        // Cutoff
        if (score >= beta) {
            // History bonus
            const i16 bonus = history::get_bonus(depth);

            if (is_quiet) {
                // Killer
                data.stack[data.ply].killer = move;

                // History
                data.history.quiet.update(data.board, move, bonus);

                for (const u16& visited : quiets) {
                    data.history.quiet.update(data.board, visited, -bonus);
                }
            }

            break;
        }

        // Adds seen move
        if (is_quiet) {
            quiets.add(move);
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

    // Updates transposition table
    u8 bound =
        best >= beta ? transposition::bound::LOWER :
        best > alpha_old ? transposition::bound::EXACT :
        transposition::bound::UPPER;

    table_entry->set(
        data.board.get_hash(),
        best_move,
        best,
        eval_static,
        depth,
        this->table.age,
        table_pv,
        bound,
        data.ply
    );

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

    // Probes transposition table
    auto [table_hit, table_entry] = this->table.get(data.board.get_hash());

    auto table_move = move::NONE;
    auto table_eval = eval::score::NONE;
    auto table_score = eval::score::NONE;
    auto table_bound = transposition::bound::NONE;
    auto table_pv = PV;

    if (table_hit) {
        table_move = table_entry->get_move();
        table_eval = table_entry->get_eval();
        table_score = table_entry->get_score(data.ply);
        table_bound = table_entry->get_bound();
        table_pv |= table_entry->is_pv();

        // Cut off
        if (!PV && table_score != eval::score::NONE) {
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= beta) ||
                (table_bound == transposition::bound::UPPER && table_score <= alpha)) {
                return table_score;
            }
        }
    }

    // Gets static eval
    i32 eval = eval::score::NONE;
    i32 eval_static = eval::score::NONE;

    if (!is_in_check) {
        eval_static = table_eval != eval::score::NONE ? table_eval : eval::get(data.board);
        eval = eval_static;

        if (table_hit) {
            // Uses the node's score as a more accurate eval value
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= beta) ||
                (table_bound == transposition::bound::UPPER && table_score <= alpha)) {
                eval = table_score;
            }
        }
        else {
            // Stores this eval into the table
            table_entry->set(
                data.board.get_hash(),
                move::NONE,
                eval::score::NONE,
                eval_static,
                0,
                this->table.age,
                table_pv,
                transposition::bound::NONE,
                data.ply
            );
        }
    }

    // Best score
    i32 best = -eval::score::INFINITE;
    u16 best_move = move::NONE;
    i32 alpha_old = alpha;

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
    auto picker = order::Picker(data, table_move, !is_in_check);
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
            best_move = move;

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

    // Updates transposition table
    u8 bound =
        best >= beta ? transposition::bound::LOWER :
        best > alpha_old ? transposition::bound::EXACT :
        transposition::bound::UPPER;

    table_entry->set(
        data.board.get_hash(),
        best_move,
        best,
        eval_static,
        0,
        this->table.age,
        table_pv,
        bound,
        data.ply
    );

    return best;
};

template i32 Engine::pvsearch<true>(Data&, i32, i32, i32);
template i32 Engine::pvsearch<false>(Data&, i32, i32, i32);

template i32 Engine::qsearch<true>(Data&, i32, i32);
template i32 Engine::qsearch<false>(Data&, i32, i32);

void init()
{
    tune::init();
};

};