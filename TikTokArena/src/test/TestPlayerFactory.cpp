#include "test/TestPlayerFactory.hpp"

#include "entities/BasicPowerUp.hpp"
#include "entities/BasicWeapon.hpp"
#include "entities/BatWeapon.hpp"
#include "entities/GunWeapon.hpp"
#include "entities/LaserWeapon.hpp"
#include "entities/RotatingWeapon.hpp"
#include "raylib.h"

#include <memory>
#include <string>
#include <vector>

namespace TikTokArena
{
    std::vector<Player> TestPlayerFactory::createRandomPlayers(int count, float radius, float initialHealth)
    {
        std::vector<Player> players;
        players.reserve(static_cast<size_t>(count));

        for (int id = 0; id < count; ++id)
        {
            players.push_back(createRandomPlayer(id, radius, initialHealth));
        }

        return players;
    }

    Player TestPlayerFactory::createPlayerWithWeapons(
        int id,
        float radius,
        float initialHealth,
        int batCount,
        int laserCount,
        int gunCount
    )
    {
        Player player{
            id,
            "Player_" + std::to_string(id + 1),
            "",
            createRandomColor(),
            radius,
            initialHealth
        };

        // Basic Attack este default pentru fiecare player.
        player.weapons.push_back(std::make_unique<BasicWeapon>());

        for (int i = 0; i < batCount; ++i)
        {
            player.weapons.push_back(std::make_unique<BatWeapon>());
        }

        for (int i = 0; i < laserCount; ++i)
        {
            player.weapons.push_back(std::make_unique<LaserWeapon>());
        }

        for (int i = 0; i < gunCount; ++i)
        {
            player.weapons.push_back(std::make_unique<GunWeapon>());
        }

        player.powerUps.push_back(std::make_unique<BasicPowerUp>());

        assignRotatingWeaponSlots(player, batCount + laserCount + gunCount);

        return player;
    }

    Player TestPlayerFactory::createRandomPlayer(int id, float radius, float initialHealth)
    {
        const int batCount = GetRandomValue(2, 3);
        const int laserCount = GetRandomValue(0, 1);
        const int gunCount = GetRandomValue(0, 1);

        return createPlayerWithWeapons(id, radius, initialHealth, batCount, laserCount, gunCount);
    }

    void TestPlayerFactory::assignRotatingWeaponSlots(Player& player, int totalRotatingWeapons)
    {
        int slotIndex = 0;

        for (auto& weapon : player.weapons)
        {
            if (auto* rotating = dynamic_cast<RotatingWeapon*>(weapon.get()))
            {
                rotating->setSlot(slotIndex, totalRotatingWeapons);
                ++slotIndex;
            }
        }
    }

    Color TestPlayerFactory::createRandomColor()
    {
        return Color{
            static_cast<unsigned char>(GetRandomValue(90, 255)),
            static_cast<unsigned char>(GetRandomValue(90, 255)),
            static_cast<unsigned char>(GetRandomValue(90, 255)),
            255
        };
    }
}
