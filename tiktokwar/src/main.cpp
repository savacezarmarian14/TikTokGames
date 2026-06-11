#include "core/Game.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct CliOptions {
    int towerCount = 3;
    tw::Config config;
};

void printUsage() {
    std::cerr << "Usage: ./tiktok_war [N] [--input-host HOST] [--input-port PORT] [--disable-network-input]\n";
    std::cerr << "N must be an integer between 2 and 4. Default: 3\n";
    std::cerr << "Network JSON input defaults to UDP 127.0.0.1:7000.\n";
}

int parseTowerCount(const std::string& value) {
    try {
        const int count = std::stoi(value);
        if (count < 2 || count > 4) {
            throw std::out_of_range("tower count must be between 2 and 4");
        }
        return count;
    } catch (const std::exception&) {
        printUsage();
        std::exit(1);
    }
}

unsigned short parsePort(const std::string& value) {
    try {
        const int port = std::stoi(value);
        if (port <= 0 || port > 65535) {
            throw std::out_of_range("port must be between 1 and 65535");
        }
        return static_cast<unsigned short>(port);
    } catch (const std::exception&) {
        printUsage();
        std::exit(1);
    }
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions options;
    bool towerCountSet = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            std::exit(0);
        }

        if (arg == "--input-host") {
            if (i + 1 >= argc) {
                printUsage();
                std::exit(1);
            }
            options.config.networkInput.host = argv[++i];
            continue;
        }

        if (arg == "--input-port") {
            if (i + 1 >= argc) {
                printUsage();
                std::exit(1);
            }
            options.config.networkInput.port = parsePort(argv[++i]);
            continue;
        }

        if (arg == "--disable-network-input") {
            options.config.networkInput.enabled = false;
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            printUsage();
            std::exit(1);
        }

        if (towerCountSet) {
            printUsage();
            std::exit(1);
        }

        options.towerCount = parseTowerCount(arg);
        towerCountSet = true;
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    const CliOptions options = parseArgs(argc, argv);
    tw::Game game(static_cast<std::size_t>(options.towerCount), options.config);
    game.run();
    return 0;
}
