#ifndef DUEL_MANAGER_HPP
#define DUEL_MANAGER_HPP

#include "core/GameConfig.hpp"
#include "duel/Duel.hpp"
#include "entities/Player.hpp"
#include "physics/PhysicsSystem.hpp"
#include "ui/HealthBarRenderer.hpp"
#include "raylib.h"

#include <random>
#include <vector>

namespace TikTokArena
{
    class DuelManager
    {
    public:
        DuelManager(
            PhysicsConfig physicsConfig,
            PlayerSpawnConfig playerSpawnConfig,
            HealthBarStyle healthBarStyle
        );

        void startDuels(std::vector<Player>& waitingPlayers, const std::vector<Rectangle>& arenas);
        void update(float deltaTime);
        void draw() const;
        void clearDuels();

        bool hasActiveDuels() const;

    private:
        void configurePlayersForArena(Player& leftPlayer, Player& rightPlayer, const Rectangle& arena) const;
        Vector2 createInitialVelocity(SpawnSide spawnSide, float speed) const;
        float randomFloat(float min, float max);

        void applyPlayerCollisionDamage(Player& first, Player& second) const;
        float getPlayerCollisionDamage(const Player& player) const;

        void drawPlayer(const Player& player) const;
        void drawWinnerText(const Duel& duel) const;

    private:
        PlayerSpawnConfig playerSpawnConfig_;
        PhysicsSystem physicsSystem_;
        HealthBarRenderer healthBarRenderer_;
        std::vector<Duel> activeDuels_;
        mutable std::mt19937 rng_{std::random_device{}()};
    };
}

#endif
