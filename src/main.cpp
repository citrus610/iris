#include "engine/search.h"
#include "test/test.h"
#include "datagen/datagen.h"

int main(int argc, char* argv[])
{
    chess::init();
    search::init();

    if constexpr (datagen::GENERATING) {
        u64 thread_count = 4;

        if (argc > 1) {
            thread_count = std::stoull(argv[1]);
        }

        datagen::run(thread_count);

        return 0;
    }

    if (argc > 1 && std::string(argv[1]) == "bench") {
        test::bench::test();
        return 0;
    }

    auto board = Board();
    auto setoption = uci::parse::Setoption();
    auto go = uci::parse::Go();
    auto engine = search::Engine();

    const std::string VERSION = "v3.0";
    const std::string NAME = "Iris";
    const std::string AUTHOR = "citrus610";

    engine.set({ .hash = 16, .threads = 1 });

    while (true)
    {
        // Gets input
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        // Splits into tokens
        std::stringstream ss(input);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ' '))
        {
            tokens.push_back(token);
        }

        // Reads input
        if (tokens[0] == "uci") {
            std::cout << "id name " << NAME << " " << VERSION << std::endl;
            std::cout << "id author " << AUTHOR << std::endl;

            uci::print::option();

            std::cout << "uciok" << std::endl;

            continue;
        }

        if (tokens[0] == "setoption") {
            auto uci_setoption = uci::parse::setoption(input);

            if (!uci_setoption.has_value()) {
                std::cout << "Invalid option!" << std::endl;
                continue;
            }

            setoption = uci_setoption.value();

            engine.set(setoption);

            continue;
        }

        if (tokens[0] == "isready") {
            std::cout << "readyok" << std::endl;

            continue;
        }

        if (tokens[0] == "ucinewgame") {
            board = Board();
            go = uci::parse::Go();

            engine.stop();
            engine.clear();

            continue;
        }

        if (tokens[0] == "position") {
            auto uci_board = uci::parse::position(input);

            if (!uci_board.has_value()) {
                std::cout << "Invalid position!" << std::endl;
                continue;
            }

            board = uci_board.value();

            continue;
        }

        if (tokens[0] == "perft") {
            if (tokens.size() < 2) {
                std::cout << "Invalid perft command!" << std::endl;
                continue;
            }

            test::perft::get<true>(board, std::stoi(tokens[1]));

            continue;
        }

        if (tokens[0] == "go") {
            // Reads go infos
            auto uci_go = uci::parse::go(input);

            if (!uci_go.has_value()) {
                std::cout << "Invalid go command!" << std::endl;
                continue;
            }

            go = uci_go.value();

            // Stops thread
            engine.stop();

            // Starts search thread
            engine.search<false>(board, go);

            continue;
        }

        if (tokens[0] == "stop") {
            // Stops thread
            engine.stop();

            continue;
        }

        if (tokens[0] == "quit" || tokens[0] == "exit") {
            // Stops thread
            engine.stop();

            break;
        }

        if (tune::TUNING && tokens[0] == "spsa") {
            tune::print_spsa();

            continue;
        }

        std::cout << "Unknown command: " << tokens[0] << std::endl;
    }
    
    return 0;
};