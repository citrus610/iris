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
    this->threads.clear();
    this->timer.clear();
    this->table.clear();
    this->nodes = 0;
    this->time = 0;
};

void Engine::set(uci::parse::Setoption uci_setoption)
{
    this->table.init(uci_setoption.hash);
    this->thread_count = uci_setoption.threads;
};

bool Engine::stop()
{
    if (this->threads.empty()) {
        return false;
    }

    this->running.clear();

    return this->join();
};

bool Engine::join()
{
    if (this->threads.empty()) {
        return false;
    }

    for (auto& t : this->threads) {
        if (!t.joinable()) {
            continue;
        }

        t.join();
    }

    this->threads.clear();

    return true;
};

template <bool BENCH>
bool Engine::search(Board uci_board, uci::parse::Go uci_go)
{
    if (this->running.test() || !this->threads.empty()) {
        return false;
    }

    // Updates data
    this->table.update();
    this->timer.set(uci_go, uci_board.get_color());
    this->nodes = 0;
    this->time = 0;

    // Starts the search thread
    this->running.test_and_set();

    // Starts threads
    for (u64 i = 0; i < this->thread_count; ++i) {
        this->threads.emplace_back([&] (Board board, uci::parse::Go go, u64 id) {
            // Inits search data
            auto data = new Data(board, id);

            // Search history
            std::vector<pv::Line> pv_history = {};
            i32 score_old = -eval::score::INFINITE;

            // Time scalers
            i32 pv_stability = 0;

            // Iterative deepening
            for (i32 i = 1; i < go.depth; ++i) {
                // Clear search data
                data->clear();

                // Principle variation search
                u64 time_1 = timer::get_current();
                i32 score = this->aspiration_window(*data, i, score_old);
                u64 time_2 = timer::get_current();

                // Avoids returning false score when stopping early
                if (!this->running.test()) {
                    score = score_old;
                }
                
                // Updates score
                score_old = score;

                // Saves pv line
                if (data->stack[0].pv.count != 0 && data->stack[0].pv[0] != move::NONE) {
                    pv_history.push_back(data->stack[0].pv);
                }

                // Saves search stats
                this->nodes += data->nodes;

                if (id == 0) {
                    this->time += time_2 - time_1;
                }

                // Prints infos
                if (!BENCH && id == 0) {
                    uci::print::info(
                        i,
                        data->seldepth,
                        wdl::get_score_normalized(score, wdl::get_material(board)),
                        data->nodes,
                        this->nodes.load() * 1000 / std::max(this->time.load(), u64(1)),
                        this->table.hashfull(),
                        pv_history.back()
                    );
                };

                // Avoids searching too shallow
                if (i < 4) {
                    continue;
                }

                // Time control scaling
                // Nodes count
                f64 nodes_ratio = f64(data->counter.get(data->stack[0].pv[0])) / f64(data->nodes);

                // PV stability
                if (pv_history.size() > 1) {
                    if (pv_history[pv_history.size() - 1][0] == pv_history[pv_history.size() - 2][0]) {
                        pv_stability = std::min(pv_stability + 1, 10);
                    }
                    else {
                        pv_stability = 0;
                    }
                }

                // Checks time
                if (id == 0 && !go.infinite && this->timer.is_over_soft(nodes_ratio, pv_stability)) {
                    this->running.clear();
                }

                if (!this->running.test()) {
                    break;
                }
            }

            // Free data
            delete data;

            // Prints best move
            if (!BENCH && id == 0) {
                uci::print::best(pv_history.back()[0]);
            };
        }, uci_board, uci_go, i);
    }

    return true;
};

i32 Engine::aspiration_window(Data& data, i32 depth, i32 score_old)
{
    i32 score = -eval::score::INFINITE;
    i32 delta = tune::AW_DELTA;
    i32 reduction = 0;

    // Sets the window
    i32 alpha = -eval::score::INFINITE;
    i32 beta = eval::score::INFINITE;

    if (depth >= tune::AW_DEPTH) {
        alpha = std::max(score_old - delta, -eval::score::INFINITE);
        beta = std::min(score_old + delta, eval::score::INFINITE);
    }

    // Loops
    while (true)
    {
        // Principle variation search
        score = this->pvsearch<node::Type::ROOT>(data, alpha, beta, std::max(depth - reduction, 1), false);

        // Aborts
        if (!this->running.test()) {
            break;
        }

        // Updates window
        if (score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha = std::max(score - delta, -eval::score::INFINITE);
            reduction = 0;
        }
        else if (score >= beta) {
            beta = std::min(score + delta, eval::score::INFINITE);
            reduction += 1;
        }
        else {
            break;
        }

        // Increase delta
        delta = delta + delta / 2;
    }

    return score;
};

template <node::Type NODE>
i32 Engine::pvsearch(Data& data, i32 alpha, i32 beta, i32 depth, bool is_cut)
{
    // Gets node type
    constexpr bool is_root = NODE == node::Type::ROOT;
    constexpr bool is_pv = NODE != node::Type::NORMAL;

    // Clears pv
    data.stack[data.ply].pv.count = 0;

    // Checks upcomming repetition
    if (!is_root && alpha < eval::score::DRAW && data.board.has_upcomming_repetition()) {
        alpha = eval::score::DRAW;

        if (alpha >= beta) {
            return alpha;
        }
    }

    // Quiensence search
    if (depth <= 0) {
        return this->qsearch<is_pv>(data, alpha, beta);
    }

    // Aborts search
    if (data.id == 0 && (data.nodes & 0xFFF) == 0 && data.nodes > 0 && this->timer.is_over_hard()) {
        this->running.clear();
    }

    if (!this->running.test()) {
        return eval::score::DRAW;
    }

    // In check
    const bool is_in_check = data.board.get_checkers();

    // Max ply reached
    if (data.ply >= MAX_PLY) {
        return is_in_check ? eval::score::DRAW : eval::get(data.board, data.nnue);
    }

    // Updates stat
    data.nodes += 1;
    data.seldepth = std::max(data.seldepth, data.ply);

    // Early stop conditions
    if (!is_root) {
        // Checks draw
        if (data.board.is_draw(data.ply)) {
            return eval::score::DRAW;
        }

        // Mate distance pruning
        alpha = std::max(alpha, data.ply - eval::score::MATE);
        beta = std::min(beta, eval::score::MATE - data.ply - 1);

        if (alpha >= beta) {
            return alpha;
        }
    }

    // Checks singular
    const bool is_singular = data.stack[data.ply].excluded != move::NONE;

    // Probes transposition table
    auto [table_hit, table_entry] = this->table.get(data.board.get_hash());

    auto table_move = move::NONE;
    auto table_eval = eval::score::NONE;
    auto table_score = eval::score::NONE;
    auto table_depth = 0;
    auto table_bound = transposition::bound::NONE;
    auto table_pv = is_pv;

    if (table_hit) {
        table_move = table_entry->get_move();
        table_eval = table_entry->get_eval();
        table_score = table_entry->get_score(data.ply);
        table_depth = table_entry->get_depth();
        table_bound = table_entry->get_bound();
        table_pv |= table_entry->is_pv();

        // Cutoff
        if (!is_pv && !is_singular && table_score != eval::score::NONE && table_depth >= depth && data.board.get_halfmove_count() < 90) {
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= beta) ||
                (table_bound == transposition::bound::UPPER && table_score <= alpha)) {
                return table_score;
            }
        }
    }

    // Gets important values for search, prunings, extensions
    // Resets killer move
    data.stack[data.ply + 1].killer = move::NONE;

    // Improving
    bool is_improving = false;

    // Static eval
    i32 eval = eval::score::NONE;
    i32 eval_raw = eval::score::NONE;
    i32 eval_static = eval::score::NONE;

    if (is_in_check) {
        data.stack[data.ply].eval = eval::score::NONE;
        goto loop;
    }
    else if (is_singular) {
        eval = data.stack[data.ply].eval;
        eval_raw = data.stack[data.ply].eval;
        eval_static = data.stack[data.ply].eval;
    }
    else {
        eval_raw = table_eval != eval::score::NONE ? table_eval : eval::get(data.board, data.nnue);
        eval_static = eval::get_adjusted(eval_raw, data.history.get_correction(data.board), data.board.get_halfmove_count());
        eval = eval_static;

        if (table_hit) {
            // Uses the node's score as a more accurate eval value
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= eval) ||
                (table_bound == transposition::bound::UPPER && table_score <= eval)) {
                eval = table_score;
            }
        }
        else {
            // Stores this eval into the table
            table_entry->set(
                data.board.get_hash(),
                move::NONE,
                eval::score::NONE,
                eval_raw,
                0,
                this->table.age,
                table_pv,
                transposition::bound::NONE,
                data.ply
            );
        }
    }

    data.stack[data.ply].eval = eval_static;

    // Improving
    if (data.ply >= 2 && data.stack[data.ply - 2].eval != eval::score::NONE) {
        is_improving = data.stack[data.ply].eval > data.stack[data.ply - 2].eval;
    }

    // Static eval quiet history
    if (!is_root &&
        data.stack[data.ply - 1].move != move::NONE &&
        data.stack[data.ply - 1].eval != eval::score::NONE &&
        data.stack[data.ply - 1].is_quiet) {
        // Gets bonus based on static eval
        const i32 delta = -(eval_static + data.stack[data.ply - 1].eval) + tune::SEQH_OFFSET;
        const i32 bonus = std::clamp(delta * tune::SEQH_COEF / 32, -tune::SEQH_MAX, tune::SEQH_MAX);

        // Updates quiet history
        data.history.quiet.update(!data.board.get_color(), data.board.get_threats_previous(), data.stack[data.ply - 1].move, bonus);
    }

    // Pruning
    if (!is_pv && !is_singular) {
        // Hindsight adjustment
        if (data.stack[data.ply - 1].reduction >= tune::HINT_ADJ_DEPTH &&
            data.stack[data.ply - 1].eval != eval::score::NONE &&
            data.stack[data.ply - 1].eval + eval_static < 0) {
            depth += 1;
        }

        // Razoring
        if (alpha < 2000 && eval + tune::RAZOR_COEF * depth < alpha) {
            // Scouts with qsearch
            const i32 score = this->qsearch<false>(data, alpha, alpha + 1);

            if (score <= alpha) {
                return score;
            }
        }

        // Reverse futility pruning
        const i32 rfp_margin =
            tune::RFP_COEF * depth -
            tune::RFP_COEF_IMP * is_improving +
            tune::RFP_BASE;

        if (depth <= tune::RFP_DEPTH &&
            eval < eval::score::MATE_FOUND &&
            eval >= beta + rfp_margin) {
            return eval;
        }

        // Null move pruning
        if (data.stack[data.ply - 1].move != move::NONE &&
            eval >= beta &&
            depth >= tune::NMP_DEPTH &&
            data.board.has_non_pawn(data.board.get_color())) {
            // Prefetch table
            this->table.prefetch(data.board.get_hash_after(move::NONE));

            // Calculates reduction count based on depth and eval
            i32 reduction = tune::NMP_RED + depth / tune::NMP_DIV_DEPTH + std::min((eval - beta) / tune::NMP_DIV_EVAL, i32(tune::NMP_RED_EVAL_MAX));
            
            // Makes null move
            data.make_null();

            // Scouts
            i32 score = -this->pvsearch<node::Type::NORMAL>(data, -beta, -beta + 1, depth - reduction, false);

            // Unmakes
            data.unmake_null();

            // Returns score if fail high, we don't return false mate score
            if (score >= beta) {
                return score < eval::score::MATE_FOUND ? score : beta;
            }
        }
    }

    // Internal iterative reduction
    if (depth >= tune::IIR_DEPTH + is_cut * tune::IIR_COEF_CUT && !table_move && (is_pv || is_cut)) {
        depth -= 1;
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
    auto noisies = arrayvec<u16, move::MAX>();

    // Iterates moves
    while (true)
    {
        // Gets move
        const u16 move = picker.get(data);

        if (!move) {
            break;
        }

        // Checks singular
        if (move == data.stack[data.ply].excluded) {
            continue;
        }

        // Checks legality
        if (!data.board.is_legal(move)) {
            continue;
        }

        legals += 1;

        // Checks for quiet
        const bool is_quiet = data.board.is_quiet(move);

        data.stack[data.ply].is_quiet = is_quiet;

        // Gets history score
        const i32 history =
            is_quiet ?
            data.history.quiet.get(data.board.get_color(), data.board.get_threats(), move) + data.history.cont.get(data, move, 1) + data.history.cont.get(data, move, 2) :
            data.history.noisy.get(data.board, move);

        // Gets reduction
        i32 reduction = tune::LMR_TABLE[depth][legals];

        reduction -= history / (is_quiet ? tune::LMR_HIST_QUIET_DIV : tune::LMR_HIST_NOISY_DIV);

        // Pruning
        if (!is_root && best > -eval::score::MATE_FOUND) {
            // Late move pruning
            if (!picker.is_skipped() &&
                legals >= (depth * depth + tune::LMP_BASE) / (2 - is_improving)) {
                picker.skip_quiets();
            }

            // Gets reduced depth
            i32 depth_reduced = std::max(0, depth - reduction);
            
            // Futility pruning
            const i32 futility = eval_static + depth_reduced * tune::FP_COEF + tune::FP_BIAS;

            if (!picker.is_skipped() &&
                !is_in_check &&
                is_quiet &&
                depth_reduced <= tune::FP_DEPTH &&
                futility <= alpha) {
                if (std::abs(best) < eval::score::MATE_FOUND && best < futility) {
                    best = futility;
                }
                    
                picker.skip_quiets();

                continue;
            }

            // SEE pruning
            i32 see_margin =
                is_quiet ?
                tune::SEEP_MARGIN_QUIET * depth_reduced :
                tune::SEEP_MARGIN_NOISY * depth_reduced * depth_reduced;
            
            if (picker.get_stage() > order::Stage::KILLER && !see::is_ok(data.board, move, see_margin)) {
                continue;
            }
        }

        // Extensions
        i32 extension = 0;

        // Singular extension
        if (!is_root &&
            !is_singular &&
            move == table_move &&
            depth >= tune::SE_DEPTH &&
            table_depth >= depth - 3 &&
            table_bound != transposition::bound::UPPER &&
            std::abs(table_score) < eval::score::MATE_FOUND) {
            // Gets search data
            const i32 singular_beta = std::max(-eval::score::INFINITE + 1, table_score - depth * 2);
            const i32 singular_depth = (depth - 1) / 2;

            // Adds excluded move
            data.stack[data.ply].excluded = move;

            // Search
            i32 score = this->pvsearch<node::Type::NORMAL>(data, singular_beta - 1, singular_beta, singular_depth, is_cut);

            // Removes excluded move
            data.stack[data.ply].excluded = move::NONE;

            // Extensions
            if (score < singular_beta) {
                // Singular extension
                extension = 1;

                // Double extension
                if (!is_pv && score + tune::SE_DOUBLE_BIAS < singular_beta) {
                    extension += 1;
                }

                // Triple extension
                if (!is_pv && is_quiet && score + tune::SE_TRIPLE_BIAS < singular_beta) {
                    extension += 1;
                }
            }
            // Multicut
            else if (singular_beta >= beta) {
                return singular_beta;
            }
            // Negative extension
            else if (table_score >= beta) {
                extension = -1;
            }
            else if (is_cut) {
                extension = -1;
            }
        }
        else {
            extension = is_in_check;
        }

        // Prefetch table
        this->table.prefetch(data.board.get_hash_after(move));

        // Gets move's nodes count
        u64 nodes_start = data.nodes;

        // Makes move
        data.make(move);

        // Searches
        i32 score = -eval::score::INFINITE;
        i32 depth_next = depth - 1 + extension;

        // Late move reduction
        if (legals > 1 + is_root * 2 &&
            depth >= tune::LMR_DEPTH &&
            picker.get_stage() > order::Stage::KILLER) {
            // Updates reduction
            reduction -= table_pv;
            reduction -= data.board.get_checkers() != 0ULL;
            reduction -= move == data.stack[data.ply - 1].killer;
            reduction += !is_improving;
            reduction += is_cut;
            reduction += data.ply >= 2 && data.stack[data.ply - 2].move == move::NONE;

            // Clamps depth to avoid qsearch
            i32 depth_reduced = std::min(std::max(depth_next - reduction, 1), depth_next);

            // Sets stack reduction
            data.stack[data.ply].reduction = reduction;

            // Scouts
            score = -this->pvsearch<node::Type::NORMAL>(data, -alpha - 1, -alpha, depth_reduced, true);

            // Resets stack reduction
            data.stack[data.ply].reduction = 0;

            // Failed
            if (score > alpha && depth_reduced < depth_next) {
                // Scales next depth based on the returned search score
                depth_next += score > best + tune::LMR_MORE_COEF + depth_next * 2;
                depth_next -= score < best + depth_next;

                // Searches again
                if (depth_reduced < depth_next) {
                    score = -this->pvsearch<node::Type::NORMAL>(data, -alpha - 1, -alpha, depth_next, !is_cut);
                }
            }
        }
        // Scouts with null window for non pv nodes
        else if (!is_pv || legals > 1) {
            score = -this->pvsearch<node::Type::NORMAL>(data, -alpha - 1, -alpha, depth_next, !is_cut);
        }

        // Searches as pv node for first child or researches after scouting
        if (is_pv && (legals == 1 || score > alpha)) {
            score = -this->pvsearch<node::Type::PV>(data, -beta, -alpha, depth_next, false);
        }

        // Unmakes move
        data.unmake(move);

        // Aborts search
        if (!this->running.test()) {
            return eval::score::DRAW;
        }

        // Set root moves' nodes count
        if constexpr (is_root) {
            data.counter.set(move, data.nodes - nodes_start);
        }

        // Updates best
        if (score > best) {
            best = score;
    
            // Updates alpha
            if (score > alpha) {
                alpha = score;

                // Updates best move
                best_move = move;
    
                // Updates pv line
                data.stack[data.ply].pv.update(move, data.stack[data.ply + 1].pv);
            }
        }

        // Cutoff
        if (score >= beta) {
            // History bonus and malus
            const i16 bonus = history::get_bonus(depth) - is_cut * tune::HS_BONUS_CUT_COEF;
            const i16 malus = history::get_malus(depth);

            if (is_quiet) {
                // Killer
                data.stack[data.ply].killer = move;

                // Quiet history
                data.history.quiet.update(data.board.get_color(), data.board.get_threats(), move, bonus);
                data.history.cont.update(data, move, bonus);

                for (const u16& visited : quiets) {
                    data.history.quiet.update(data.board.get_color(), data.board.get_threats(), visited, -malus);
                    data.history.cont.update(data, visited, -malus);
                }
            }
            else {
                // Noisy history
                data.history.noisy.update(data.board, move, bonus);
            }

            // Even if the best move wasn't noisy, we still decrease the other noisy moves' history scores
            for (const u16& visited : noisies) {
                data.history.noisy.update(data.board, visited, -malus);
            }

            break;
        }

        // Adds seen moves
        if (is_quiet) {
            quiets.add(move);
        }
        else {
            noisies.add(move);
        }
    }

    // Checks stalemate or checkmate
    if (legals == 0) {
        if (is_singular) {
            return alpha;
        }

        if (is_in_check) {
            return data.ply - eval::score::MATE;
        }

        return eval::score::DRAW;
    }

    // Gets bound
    u8 bound =
        best >= beta ? transposition::bound::LOWER :
        best > alpha_old ? transposition::bound::EXACT :
        transposition::bound::UPPER;

    // Prior counter move history
    if (!is_root &&
        bound == transposition::bound::UPPER &&
        (quiets.size() > 0 || depth > 3) &&
        data.stack[data.ply - 1].move != move::NONE &&
        data.stack[data.ply - 1].is_quiet) {
        // Bonus
        const i16 bonus = history::get_bonus(depth);

        // Updates history
        data.history.quiet.update(!data.board.get_color(), data.board.get_threats_previous(), data.stack[data.ply - 1].move, bonus);
    }

    // Updates correction history
    if ((best_move == move::NONE || data.board.is_quiet(best_move)) &&
        !is_in_check &&
        !(bound == transposition::bound::LOWER && best <= eval_static) &&
        !(bound == transposition::bound::UPPER && best >= eval_static)) {
        // Gets correction bonus
        const i16 bonus = history::corr::get_bonus(best - eval_static, depth);

        // Updates
        data.history.update_correction(data.board, bonus);
    }

    // Updates transposition table
    if (!is_singular) {
        table_entry->set(
            data.board.get_hash(),
            best_move,
            best,
            eval_raw,
            depth,
            this->table.age,
            table_pv,
            bound,
            data.ply
        );
    }

    return best;
};

template <bool PV>
i32 Engine::qsearch(Data& data, i32 alpha, i32 beta)
{
    // Clears pv
    data.stack[data.ply].pv.count = 0;

    // Aborts search
    if (data.id == 0 && (data.nodes & 0xFFF) == 0 && data.nodes > 0 && this->timer.is_over_hard()) {
        this->running.clear();
    }

    if (!this->running.test()) {
        return eval::score::DRAW;
    }

    // In check
    const bool is_in_check = data.board.get_checkers();

    // Max ply reached
    if (data.ply >= MAX_PLY) {
        return is_in_check ? eval::score::DRAW : eval::get(data.board, data.nnue);
    }

    // Updates stat
    data.nodes += 1;
    data.seldepth = std::max(data.seldepth, data.ply);

    // Checks draw
    if (data.board.is_draw(data.ply)) {
        return eval::score::DRAW;
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
    i32 eval_raw = eval::score::NONE;
    i32 eval_static = eval::score::NONE;

    if (!is_in_check) {
        eval_raw = table_eval != eval::score::NONE ? table_eval : eval::get(data.board, data.nnue);
        eval_static = eval::get_adjusted(eval_raw, data.history.get_correction(data.board), data.board.get_halfmove_count());
        eval = eval_static;

        if (table_hit) {
            // Uses the node's score as a more accurate eval value
            if ((table_bound == transposition::bound::EXACT) ||
                (table_bound == transposition::bound::LOWER && table_score >= eval) ||
                (table_bound == transposition::bound::UPPER && table_score <= eval)) {
                eval = table_score;
            }
        }
        else {
            // Stores this eval into the table
            table_entry->set(
                data.board.get_hash(),
                move::NONE,
                eval::score::NONE,
                eval_raw,
                0,
                this->table.age,
                table_pv,
                transposition::bound::NONE,
                data.ply
            );
        }
    }

    data.stack[data.ply].eval = eval_static;

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

        // Pruning
        if (best > -eval::score::MATE_FOUND) {
            // Baddies pruning
            if (picker.get_stage() == order::Stage::NOISY_BAD) {
                break;
            }

            // Futility pruning
            if (!is_in_check) {
                const i32 futility = data.stack[data.ply].eval + tune::FP_MARGIN_QS;

                if (futility <= alpha && !see::is_ok(data.board, move, 1)) {
                    best = std::max(best, futility);
                    continue;
                }
            }

            // SEE pruning
            if (!see::is_ok(data.board, move, tune::SEEP_MARGIN_QS)) {
                continue;
            }
        }

        // Prefetch table
        this->table.prefetch(data.board.get_hash_after(move));

        // Makes move
        data.make(move);

        // Searches
        i32 score = -this->qsearch<PV>(data, -beta, -alpha);

        // Unmakes move
        data.unmake(move);

        // Aborts search
        if (!this->running.test()) {
            return eval::score::DRAW;
        }

        // Skips quiet moves if we've found non-mate score
        if (is_in_check && score > -eval::score::MATE_FOUND) {
            picker.skip_quiets();
        }

        // Updates best
        if (score > best) {
            best = score;

            // Updates alpha
            if (score > alpha) {
                alpha = score;

                // Updates best move
                best_move = move;
    
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
        return data.ply - eval::score::MATE;
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
        eval_raw,
        0,
        this->table.age,
        table_pv,
        bound,
        data.ply
    );

    return best;
};

template bool Engine::search<true>(Board, uci::parse::Go);
template bool Engine::search<false>(Board, uci::parse::Go);

template i32 Engine::pvsearch<node::Type::ROOT>(Data&, i32, i32, i32, bool);
template i32 Engine::pvsearch<node::Type::PV>(Data&, i32, i32, i32, bool);
template i32 Engine::pvsearch<node::Type::NORMAL>(Data&, i32, i32, i32, bool);

template i32 Engine::qsearch<true>(Data&, i32, i32);
template i32 Engine::qsearch<false>(Data&, i32, i32);

void init()
{
    tune::init();
    nnue::init();
};

};