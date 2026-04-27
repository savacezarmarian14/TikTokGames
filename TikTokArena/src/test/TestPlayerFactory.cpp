#include "test/TestPlayerFactory.hpp"

#include "entities/BasicPowerUp.hpp"
#include "entities/BasicWeapon.hpp"
#include "entities/BatWeapon.hpp"
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
            players.push_back(createPlayer(id, radius, initialHealth));

        return players;
    }

    Player TestPlayerFactory::createPlayer(int id, float radius, float initialHealth)
    {
        Player player{id, "Player_" + std::to_string(id + 1), "", createRandomColor(), radius, initialHealth};

        // Weapon de baza (coliziune corp)
        player.weapons.push_back(std::make_unique<BasicWeapon>());

        // Alege cate rotating weapons are jucatorul
        const int batCount   = GetRandomValue(2, 3);
        const int laserCount = GetRandomValue(0, 1);
        const int totalRotating = batCount + laserCount;

        // Adauga bat-urile
        for (int i = 0; i < batCount; ++i)
            player.weapons.push_back(std::make_unique<BatWeapon>());

        // Adauga laser-ele
        for (int i = 0; i < laserCount; ++i)
            player.weapons.push_back(std::make_unique<LaserWeapon>());

        player.powerUps.push_back(std::make_unique<BasicPowerUp>());

        // Distribuie toate rotating weapons la unghiuri egale pe cerc
        // (bat si laser sunt tratate uniform — nu conteaza tipul)
        int slotIndex = 0;
        for (auto& weapon : player.weapons)
        {
            if (auto* rotating = dynamic_cast<RotatingWeapon*>(weapon.get()))
            {
                rotating->setSlot(slotIndex, totalRotating);
                ++slotIndex;
            }
        }

        return player;
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
