#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "entities/PowerUp.hpp"
#include "entities/Weapon.hpp"
#include "raylib.h"

#include <memory>
#include <string>
#include <vector>

namespace TikTokArena
{
    enum class SpawnSide
    {
        Left,
        Right
    };

    class Player
    {
    public:
        Player() = default;
        Player(int id, std::string username, std::string photoPath, Color color, float radius, float maxHealth);

        Player(const Player&) = delete;
        Player& operator=(const Player&) = delete;
        Player(Player&&) noexcept = default;
        Player& operator=(Player&&) noexcept = default;

        void applyDamage(float damage);
        bool isAlive() const;

        int id{};
        std::string username;
        std::string photoPath;

        Vector2 position{};
        Vector2 velocity{};
        SpawnSide spawnSide{SpawnSide::Left};

        Color color{255, 255, 255, 255};
        float radius{};

        float health{};
        float maxHealth{};

        std::vector<std::unique_ptr<Weapon>> weapons;
        std::vector<std::unique_ptr<PowerUp>> powerUps;
    };
}

#endif
