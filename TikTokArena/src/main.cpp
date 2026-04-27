#include "TikTokArena.hpp"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        TikTokArena::Game game;
        return game.start();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[FATAL] " << exception.what() << std::endl;
        return 1;
    }
}
