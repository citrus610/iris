#include "engine/search.h"
#include "test/test.h"

int main()
{
    chess::init();

    auto board = Board();
    auto setoption = uci::parse::Setoption();
    auto go = uci::parse::Go();
    auto engine = search::Engine();

    const std::string NAME = "Iris v0.1";
    const std::string AUTHOR = "citrus610";

    engine.set({ .hash = 16 });

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
            std::cout << "id name " << NAME << std::endl;
            std::cout << "id author " << AUTHOR << std::endl;
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
            engine.search(board, go);

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

        std::cout << "Unknown command: " << tokens[0] << std::endl;
    }
    
    return 0;
};