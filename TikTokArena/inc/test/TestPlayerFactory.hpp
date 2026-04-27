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

        static Player createPlayerWithWeapons(
            int id,
            float radius,
            float initialHealth,
            int batCount,
            int laserCount,
            int gunCount
        );

    private:
        static Player createRandomPlayer(int id, float radius, float initialHealth);
        static void assignRotatingWeaponSlots(Player& player, int totalRotatingWeapons);
        static Color createRandomColor();
    };
}

#endif
