#ifndef DUEL_MANAGER_HPP
#define DUEL_MANAGER_HPP

#include "core/GameConfig.hpp"
#include "duel/Duel.hpp"
#include "entities/Player.hpp"
#include "physics/PhysicsSystem.hpp"
#include "ui/HealthBarRenderer.hpp"
#include "raylib.h"

#include <random>
#include <string>
#include <vector>

namespace TikTokArena
{
    struct WinnerResult
    {
        std::string playerName;
        float elapsedTime{};
    };

    class DuelManager
    {
    public:
        DuelManager(
            PhysicsConfig physicsConfig,
            PlayerSpawnConfig playerSpawnConfig,
            HealthBarStyle healthBarStyle
        );

        void prepareDuels(std::vector<Player>& waitingPlayers, const std::vector<Rectangle>& arenas);
        void activatePreparedDuels();

        void update(float deltaTime);
        void draw() const;
        void clearDuels();

        bool hasActiveDuels() const;
        bool areAllDuelsFinished() const;
        std::vector<WinnerResult> getWinnerResultsSortedByTime() const;

    private:
        void configurePlayersForArena(Player& leftPlayer, Player& rightPlayer, const Rectangle& arena) const;
        void resetPlayerForDuel(Player& player) const;
        Vector2 createInitialVelocity(SpawnSide spawnSide, float speed) const;
        float randomFloat(float min, float max);

        void applyPlayerCollisionDamage(Player& first, Player& second) const;
        float getPlayerCollisionDamage(const Player& player) const;

        void drawPlayer(const Player& player) const;
        void drawWinnerText(const Duel& duel) const;
        void drawArenaTimer(const Duel& duel) const;
        std::string formatElapsedTime(float elapsedTime) const;

        void drawPreparedDuel(const Duel& duel) const;
        void drawWeaponListBox(const Player& player, const Rectangle& box) const;
        std::vector<std::string> buildWeaponList(const Player& player) const;

    private:
        PlayerSpawnConfig playerSpawnConfig_;
        PhysicsSystem physicsSystem_;
        HealthBarRenderer healthBarRenderer_;
        std::vector<Duel> activeDuels_;
        mutable std::mt19937 rng_{std::random_device{}()};
    };
}

#endif
