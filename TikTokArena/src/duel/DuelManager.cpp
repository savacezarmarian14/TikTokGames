#include "duel/DuelManager.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace TikTokArena
{
    DuelManager::DuelManager(
        PhysicsConfig physicsConfig,
        PlayerSpawnConfig playerSpawnConfig,
        HealthBarStyle healthBarStyle
    )
        : playerSpawnConfig_(playerSpawnConfig),
          physicsSystem_(physicsConfig),
          healthBarRenderer_(healthBarStyle)
    {
    }

    void DuelManager::startDuels(std::vector<Player>& waitingPlayers, const std::vector<Rectangle>& arenas)
    {
        // Curata runda anterioara - jucatorii finished dispar, fara return
        activeDuels_.clear();

        const size_t maxDuelCount = std::min(arenas.size(), waitingPlayers.size() / 2U);

        if (maxDuelCount == 0U)
        {
            return;
        }

        std::shuffle(
            waitingPlayers.begin(),
            waitingPlayers.end(),
            rng_
        );

        for (size_t duelIndex = 0; duelIndex < maxDuelCount; ++duelIndex)
        {
            Player leftPlayer = std::move(waitingPlayers.back());
            waitingPlayers.pop_back();

            Player rightPlayer = std::move(waitingPlayers.back());
            waitingPlayers.pop_back();

            configurePlayersForArena(leftPlayer, rightPlayer, arenas[duelIndex]);

            activeDuels_.emplace_back(
                std::move(leftPlayer),
                std::move(rightPlayer),
                arenas[duelIndex]
            );
        }
    }

    void DuelManager::update(float deltaTime)
    {
        for (Duel& duel : activeDuels_)
        {
            if (duel.getState() == DuelState::Finished)
            {
                continue;
            }

            Player& leftPlayer = duel.getLeftPlayer();
            Player& rightPlayer = duel.getRightPlayer();

            const bool playerCollision = physicsSystem_.update(
                leftPlayer,
                rightPlayer,
                duel.getArenaBounds(),
                deltaTime
            );

            if (playerCollision)
            {
                applyPlayerCollisionDamage(leftPlayer, rightPlayer);
            }

            for (auto& weapon : leftPlayer.weapons)
            {
                if (weapon) weapon->update(leftPlayer, rightPlayer, deltaTime);
            }

            for (auto& weapon : rightPlayer.weapons)
            {
                if (weapon) weapon->update(rightPlayer, leftPlayer, deltaTime);
            }

            if (!leftPlayer.isAlive() || !rightPlayer.isAlive())
            {
                const std::string winnerName =
                    leftPlayer.isAlive() ? leftPlayer.username : rightPlayer.username;
                duel.finish(winnerName);
            }
        }
    }

    void DuelManager::clearDuels()
    {
        activeDuels_.clear();
    }

    void DuelManager::draw() const
    {
        for (const Duel& duel : activeDuels_)
        {
            drawPlayer(duel.getLeftPlayer());
            drawPlayer(duel.getRightPlayer());

            if (duel.getState() == DuelState::Finished)
            {
                drawWinnerText(duel);
            }
        }
    }

    bool DuelManager::hasActiveDuels() const
    {
        return !activeDuels_.empty();
    }

    void DuelManager::configurePlayersForArena(
        Player& leftPlayer,
        Player& rightPlayer,
        const Rectangle& arena
    ) const
    {
        const float baseUnit = std::min(arena.width, arena.height);
        const float radius = baseUnit * playerSpawnConfig_.radiusRatio;
        const float speed = baseUnit * playerSpawnConfig_.speedRatio;
        const float horizontalOffset = arena.width * playerSpawnConfig_.horizontalSpawnRatio;
        const float middleY = arena.y + arena.height * 0.5f;

        // Reset health
        leftPlayer.health = playerSpawnConfig_.initialHealth;
        leftPlayer.maxHealth = playerSpawnConfig_.initialHealth;
        rightPlayer.health = playerSpawnConfig_.initialHealth;
        rightPlayer.maxHealth = playerSpawnConfig_.initialHealth;

        // Reset arme (wasColliding etc.)
        for (auto& weapon : leftPlayer.weapons)
            if (weapon) weapon->reset();
        for (auto& weapon : rightPlayer.weapons)
            if (weapon) weapon->reset();

        leftPlayer.radius = radius;
        leftPlayer.spawnSide = SpawnSide::Left;
        leftPlayer.position = Vector2{arena.x + horizontalOffset, middleY};
        leftPlayer.velocity = createInitialVelocity(SpawnSide::Left, speed);

        rightPlayer.radius = radius;
        rightPlayer.spawnSide = SpawnSide::Right;
        rightPlayer.position = Vector2{arena.x + arena.width - horizontalOffset, middleY};
        rightPlayer.velocity = createInitialVelocity(SpawnSide::Right, speed);
    }

    Vector2 DuelManager::createInitialVelocity(SpawnSide spawnSide, float speed) const
    {
        const float maxOffsetRadians =
            playerSpawnConfig_.maxInitialAngleOffsetDegrees * DEG2RAD;

        std::uniform_real_distribution<float> dist(-maxOffsetRadians, maxOffsetRadians);
        const float angleOffset = dist(rng_);
        const float baseAngle = spawnSide == SpawnSide::Left ? 0.0f : PI;
        const float angle = baseAngle + angleOffset;

        return Vector2{std::cos(angle) * speed, std::sin(angle) * speed};
    }

    float DuelManager::randomFloat(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng_);
    }

    void DuelManager::applyPlayerCollisionDamage(Player& first, Player& second) const
    {
        const float firstDamage = getPlayerCollisionDamage(first);
        const float secondDamage = getPlayerCollisionDamage(second);
        second.applyDamage(firstDamage);
        first.applyDamage(secondDamage);
    }

    float DuelManager::getPlayerCollisionDamage(const Player& player) const
    {
        float totalDamage = 0.0f;
        for (const auto& weapon : player.weapons)
            if (weapon) totalDamage += weapon->getPlayerCollisionDamage();
        return totalDamage;
    }

    void DuelManager::drawWinnerText(const Duel& duel) const
    {
        const Rectangle arena = duel.getArenaBounds();
        const std::string text = duel.getWinnerName() + " WON";
        const int fontSize = static_cast<int>(arena.height * 0.12f);
        const int textWidth = MeasureText(text.c_str(), fontSize);

        DrawText(
            text.c_str(),
            static_cast<int>(arena.x + arena.width * 0.5f - textWidth * 0.5f),
            static_cast<int>(arena.y + arena.height * 0.5f - fontSize * 0.5f),
            fontSize,
            RAYWHITE
        );
    }

    void DuelManager::drawPlayer(const Player& player) const
    {
        const int nameFontSize = static_cast<int>(player.radius * 0.45f);
        const int nameWidth = MeasureText(player.username.c_str(), nameFontSize);

        DrawText(
            player.username.c_str(),
            static_cast<int>(player.position.x - static_cast<float>(nameWidth) * 0.5f),
            static_cast<int>(player.position.y - player.radius * 1.75f),
            nameFontSize,
            RAYWHITE
        );

        DrawCircleV(player.position, player.radius, player.color);

        for (const auto& weapon : player.weapons)
            if (weapon) weapon->draw(player);

        healthBarRenderer_.draw(
            player.position,
            player.radius,
            player.health,
            player.maxHealth
        );
    }
}
