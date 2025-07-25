#include "uci.h"

namespace uci::parse
{

std::optional<u16> move(const std::string& token, Board& board)
{
    if (token.size() < 4 || token.size() > 5) {
        return {};
    }

    i8 from = square::create(
        file::create(token[0]),
        rank::create(token[1])
    );

    i8 to = square::create(
        file::create(token[2]),
        rank::create(token[3])
    );

    auto moves = move::gen::get<move::gen::type::ALL>(board);

    for (const auto move : moves) {
        if (!board.is_legal(move)) {
            continue;
        }

        i8 move_from = move::get_from(move);
        i8 move_to = move::get_to(move);
        i8 move_to_castle = move_to;

        if (move::get_type(move) == move::type::CASTLING) {
            move_to_castle = castling::get_king_to(board.get_color(), castling::create(move_to) & castling::SHORT);
        }

        if (from != move_from || (to != move_to && to != move_to_castle)) {
            continue;
        }

        if (token.size() == 5 && move::get_type(move) == move::type::PROMOTION) {
            if (token[4] == piece::type::get_char(move::get_promotion_type(move))) {
                return move;
            }

            continue;
        }

        return move;
    }

    return {};
};

std::optional<Board> position(std::string in)
{
    Board board;

    if (in.find("startpos") != std::string::npos) {
        board = Board();
    }
    else if (in.find("fen") != std::string::npos) {
        auto fen = in.substr(in.find("fen") + 4);

        board = Board(in.substr(in.find("fen") + 4, std::string::npos));
    }

    if (in.find("moves") != std::string::npos) {
        auto start = in.find("moves") + 6;
        
        if (in.length() >= start) {
            std::string moves_substr = in.substr(start, std::string::npos);

            std::stringstream ss(moves_substr);
            std::string token;

            while (std::getline(ss, token, ' '))
            {
                auto move = uci::parse::move(token, board);

                if (!move.has_value()) {
                    return {};
                }

                board.make(move.value());
            }
        }
    }

    return board;
};

std::optional<Go> go(std::string in)
{
    auto option = Go {
        .depth = MAX_PLY,
        .time = { 0, 0 },
        .increment = { 0, 0 },
        .movestogo = {},
        .infinite = true
    };

    std::stringstream ss(in);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ' '))
    {
        tokens.push_back(token);
    }

    for (usize i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == "infinite") {
            option.infinite = true;
        }

        if (tokens[i] == "winc") {
            option.increment[color::WHITE] = std::max(std::stoi(tokens[i + 1]), 0);
        }

        if (tokens[i] == "binc") {
            option.increment[color::BLACK] = std::max(std::stoi(tokens[i + 1]), 0);
        }

        if (tokens[i] == "wtime") {
            option.time[color::WHITE] = std::max(std::stoi(tokens[i + 1]), 1);
            option.infinite = false;
        }

        if (tokens[i] == "btime") {
            option.time[color::BLACK] = std::max(std::stoi(tokens[i + 1]), 1);
            option.infinite = false;
        }

        if (tokens[i] == "depth") {
            option.depth = std::max(std::stoi(tokens[i + 1]), 1);
            option.infinite = false;
        }

        if (tokens[i] == "movestogo") {
            option.movestogo = std::max(std::stoi(tokens[i + 1]), 1);
        }
    }

    return option;
};

std::optional<Setoption> setoption(std::string in)
{
    auto option = Setoption();

    std::stringstream ss(in);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.size() < 5) {
        return option;
    }

    if (tokens[2] == "Hash") {
        option.hash = std::clamp(std::stoi(tokens[4]), i32(HASH_MIN), i32(HASH_MAX));
    }

    if (tokens[2] == "Threads") {
        option.threads = std::clamp(std::stoi(tokens[4]), i32(THREAD_MIN), i32(THREAD_MAX));
    }

    if constexpr (tune::TUNING) {
        auto value = tune::find(tokens[2]);

        if (value != nullptr) {
            value->value = std::stoi(tokens[4]);
        }

        tune::init();
    }

    return option;
};

};

namespace uci::print
{

void option()
{
    std::cout << "option name Hash type spin default " << HASH_DEFAULT << " min " << HASH_MIN << " max " << HASH_MAX << std::endl;
    std::cout << "option name Threads type spin default " << THREAD_DEFAULT << " min " << THREAD_MIN << " max " << THREAD_MAX << std::endl;

    if constexpr (!tune::TUNING) {
        return;
    }

    for (auto value : tune::values) {
        std::cout << "option name " << value->name << " type spin default " << value->value << " min " << value->min << " max " << value->max << std::endl;
    }
};

void info(i32 depth, i32 seldepth, i32 score, u64 nodes, u64 nps, u64 hashfull, pv::Line pv)
{
    std::cout << "info ";

    std::cout << "depth " << depth << " ";

    std::cout << "seldepth " << seldepth << " ";

    if (score >= eval::score::MATE_FOUND) {
        std::cout << "score mate " << ((eval::score::MATE - score) / 2) << " ";
    }
    else if (score <= -eval::score::MATE_FOUND) {
        std::cout << "score mate " << ((-eval::score::MATE - score) / 2) << " ";
    }
    else {
        std::cout << "score cp " << score << " ";
    }

    std::cout << "nodes " << nodes << " ";

    std::cout << "nps " << nps << " ";

    std::cout << "hashfull " << hashfull <<  " ";

    std::cout << "pv ";
    
    for (i32 i = 0; i < pv.count; ++i) {
        std::cout << move::get_str(pv.data[i]) << " ";
    }
    
    std::cout << std::endl;
};

void best(u16 move)
{
    std::cout << "bestmove " << move::get_str(move) << std::endl;
};

};