#ifndef TEST_PLAYER_FACTORY_HPP
#define TEST_PLAYER_FACTORY_HPP

#include "entities/Player.hpp"

#include <vector>

namespace TikTokArena
{
    class TestPlayerFactory
    {
    public:
        static std::vector<Player> createRandomPlayers(int count, float radius, float initialHealth);

    private:
        static Player createPlayer(int id, float radius, float initialHealth);
        static Color createRandomColor();
    };
}

#endif
