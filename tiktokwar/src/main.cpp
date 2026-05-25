#include "core/Game.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
int parseTowerCount(int argc, char** argv) {
    if (argc < 2) {
        return 3;
    }

    try {
        const int value = std::stoi(argv[1]);
        if (value < 2) {
            throw std::out_of_range("tower count must be >= 2");
        }
        return value;
    } catch (const std::exception&) {
        std::cerr << "Usage: ./tiktok_war [N]\n";
        std::cerr << "N must be an integer >= 2\n";
        std::exit(1);
    }
}
}

int main(int argc, char** argv) {
    const int towerCount = parseTowerCount(argc, argv);
    tw::Game game(static_cast<std::size_t>(towerCount));
    game.run();
    return 0;
}