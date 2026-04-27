#include "duel/DuelManager.hpp"

#include "entities/Weapon.hpp"
#include "entities/GunWeapon.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

    void DuelManager::prepareDuels(std::vector<Player>& waitingPlayers, const std::vector<Rectangle>& arenas)
    {
        activeDuels_.clear();

        const size_t maxDuelCount = std::min(arenas.size(), waitingPlayers.size() / 2U);

        if (maxDuelCount == 0U)
        {
            return;
        }

        std::shuffle(waitingPlayers.begin(), waitingPlayers.end(), rng_);

        for (size_t duelIndex = 0; duelIndex < maxDuelCount; ++duelIndex)
        {
            Player leftPlayer = std::move(waitingPlayers.back());
            waitingPlayers.pop_back();

            Player rightPlayer = std::move(waitingPlayers.back());
            waitingPlayers.pop_back();

            resetPlayerForDuel(leftPlayer);
            resetPlayerForDuel(rightPlayer);

            activeDuels_.emplace_back(
                std::move(leftPlayer),
                std::move(rightPlayer),
                arenas[duelIndex]
            );
        }
    }

    void DuelManager::activatePreparedDuels()
    {
        for (Duel& duel : activeDuels_)
        {
            if (duel.getState() != DuelState::Prepared)
            {
                continue;
            }

            configurePlayersForArena(
                duel.getLeftPlayer(),
                duel.getRightPlayer(),
                duel.getArenaBounds()
            );

            duel.start();
        }
    }

    void DuelManager::update(float deltaTime)
    {
        for (Duel& duel : activeDuels_)
        {
            if (duel.getState() != DuelState::Running)
            {
                continue;
            }

            duel.updateElapsedTime(deltaTime);

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
                if (!weapon)
                {
                    continue;
                }

                if (auto* gun = dynamic_cast<GunWeapon*>(weapon.get()))
                {
                    gun->updateWithArena(leftPlayer, rightPlayer, duel.getArenaBounds(), deltaTime);
                }
                else
                {
                    weapon->update(leftPlayer, rightPlayer, deltaTime);
                }
            }

            for (auto& weapon : rightPlayer.weapons)
            {
                if (!weapon)
                {
                    continue;
                }

                if (auto* gun = dynamic_cast<GunWeapon*>(weapon.get()))
                {
                    gun->updateWithArena(rightPlayer, leftPlayer, duel.getArenaBounds(), deltaTime);
                }
                else
                {
                    weapon->update(rightPlayer, leftPlayer, deltaTime);
                }
            }

            const bool leftAlive = leftPlayer.isAlive();
            const bool rightAlive = rightPlayer.isAlive();

            if (!leftAlive || !rightAlive)
            {
                if (!leftAlive && !rightAlive)
                {
                    duel.finish("DRAW");
                }
                else if (leftAlive)
                {
                    duel.finish(leftPlayer.username + " WON");
                }
                else
                {
                    duel.finish(rightPlayer.username + " WON");
                }
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
            if (duel.getState() == DuelState::Prepared)
            {
                drawPreparedDuel(duel);
                drawArenaTimer(duel);
                continue;
            }

            drawPlayer(duel.getLeftPlayer());
            drawPlayer(duel.getRightPlayer());
            drawArenaTimer(duel);

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

    bool DuelManager::areAllDuelsFinished() const
    {
        if (activeDuels_.empty())
        {
            return false;
        }

        for (const Duel& duel : activeDuels_)
        {
            if (duel.getState() != DuelState::Finished)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<WinnerResult> DuelManager::getWinnerResultsSortedByTime() const
    {
        std::vector<WinnerResult> results;

        for (const Duel& duel : activeDuels_)
        {
            if (duel.getState() != DuelState::Finished)
            {
                continue;
            }

            const Player& leftPlayer = duel.getLeftPlayer();
            const Player& rightPlayer = duel.getRightPlayer();

            if (!leftPlayer.isAlive() && !rightPlayer.isAlive())
            {
                continue;
            }

            if (leftPlayer.isAlive())
            {
                results.push_back(WinnerResult{leftPlayer.username, duel.getElapsedTime()});
            }
            else if (rightPlayer.isAlive())
            {
                results.push_back(WinnerResult{rightPlayer.username, duel.getElapsedTime()});
            }
        }

        std::sort(
            results.begin(),
            results.end(),
            [](const WinnerResult& first, const WinnerResult& second)
            {
                return first.elapsedTime < second.elapsedTime;
            }
        );

        return results;
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

        resetPlayerForDuel(leftPlayer);
        resetPlayerForDuel(rightPlayer);

        leftPlayer.radius = radius;
        leftPlayer.spawnSide = SpawnSide::Left;
        leftPlayer.position = Vector2{arena.x + horizontalOffset, middleY};
        leftPlayer.velocity = createInitialVelocity(SpawnSide::Left, speed);

        rightPlayer.radius = radius;
        rightPlayer.spawnSide = SpawnSide::Right;
        rightPlayer.position = Vector2{arena.x + arena.width - horizontalOffset, middleY};
        rightPlayer.velocity = createInitialVelocity(SpawnSide::Right, speed);
    }

    void DuelManager::resetPlayerForDuel(Player& player) const
    {
        player.health = playerSpawnConfig_.initialHealth;
        player.maxHealth = playerSpawnConfig_.initialHealth;
        player.velocity = Vector2{0.0f, 0.0f};

        for (auto& weapon : player.weapons)
        {
            if (weapon)
            {
                weapon->reset();
            }
        }
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
        {
            if (weapon)
            {
                totalDamage += weapon->getPlayerCollisionDamage();
            }
        }

        return totalDamage;
    }

    void DuelManager::drawWinnerText(const Duel& duel) const
    {
        const Rectangle arena = duel.getArenaBounds();
        const std::string text = duel.getResultText();
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

    void DuelManager::drawArenaTimer(const Duel& duel) const
    {
        const Rectangle arena = duel.getArenaBounds();
        const std::string timerText = formatElapsedTime(duel.getElapsedTime());

        const int fontSize = static_cast<int>(std::max(10.0f, arena.height * 0.045f));
        const int textWidth = MeasureText(timerText.c_str(), fontSize);

        const float paddingX = arena.width * 0.035f;
        const float paddingY = arena.height * 0.035f;
        const float boxPaddingX = fontSize * 0.45f;
        const float boxPaddingY = fontSize * 0.25f;

        const Rectangle box{
            arena.x + arena.width - paddingX - static_cast<float>(textWidth) - boxPaddingX * 2.0f,
            arena.y + paddingY,
            static_cast<float>(textWidth) + boxPaddingX * 2.0f,
            static_cast<float>(fontSize) + boxPaddingY * 2.0f
        };

        DrawRectangleRounded(box, 0.25f, 8, Color{0, 0, 0, 150});
        DrawRectangleRoundedLinesEx(box, 0.25f, 8, 1.5f, Color{230, 230, 245, 185});

        DrawText(
            timerText.c_str(),
            static_cast<int>(box.x + boxPaddingX),
            static_cast<int>(box.y + boxPaddingY),
            fontSize,
            RAYWHITE
        );
    }

    std::string DuelManager::formatElapsedTime(float elapsedTime) const
    {
        const int totalMilliseconds = static_cast<int>(elapsedTime * 1000.0f);
        const int minutes = totalMilliseconds / 60000;
        const int seconds = (totalMilliseconds / 1000) % 60;
        const int milliseconds = totalMilliseconds % 1000;

        std::ostringstream stream;
        stream << std::setfill('0')
               << std::setw(2) << minutes << ":"
               << std::setw(2) << seconds << ":"
               << std::setw(3) << milliseconds;

        return stream.str();
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
        {
            if (weapon)
            {
                weapon->draw(player);
            }
        }

        healthBarRenderer_.draw(
            player.position,
            player.radius,
            player.health,
            player.maxHealth
        );
    }

    void DuelManager::drawPreparedDuel(const Duel& duel) const
    {
        const Rectangle arena = duel.getArenaBounds();
        const Player& leftPlayer = duel.getLeftPlayer();
        const Player& rightPlayer = duel.getRightPlayer();

        const int titleFontSize = static_cast<int>(arena.height * 0.07f);
        const char* title = "Arena";
        const int titleWidth = MeasureText(title, titleFontSize);

        DrawText(
            title,
            static_cast<int>(arena.x + arena.width * 0.5f - static_cast<float>(titleWidth) * 0.5f),
            static_cast<int>(arena.y + arena.height * 0.06f),
            titleFontSize,
            RAYWHITE
        );

        const float avatarRadius = std::min(arena.width, arena.height) * 0.075f;

        const Vector2 leftAvatar{
            arena.x + arena.width * 0.27f,
            arena.y + arena.height * 0.23f
        };

        const Vector2 rightAvatar{
            arena.x + arena.width * 0.73f,
            arena.y + arena.height * 0.23f
        };

        DrawCircleV(leftAvatar, avatarRadius, Color{leftPlayer.color.r, leftPlayer.color.g, leftPlayer.color.b, 95});
        DrawCircleLines(
            static_cast<int>(leftAvatar.x),
            static_cast<int>(leftAvatar.y),
            avatarRadius,
            RAYWHITE
        );

        DrawCircleV(rightAvatar, avatarRadius, Color{rightPlayer.color.r, rightPlayer.color.g, rightPlayer.color.b, 95});
        DrawCircleLines(
            static_cast<int>(rightAvatar.x),
            static_cast<int>(rightAvatar.y),
            avatarRadius,
            RAYWHITE
        );

        const int nameFontSize = static_cast<int>(arena.height * 0.045f);

        const int leftNameWidth = MeasureText(leftPlayer.username.c_str(), nameFontSize);
        DrawText(
            leftPlayer.username.c_str(),
            static_cast<int>(leftAvatar.x - static_cast<float>(leftNameWidth) * 0.5f),
            static_cast<int>(leftAvatar.y - avatarRadius - nameFontSize - 6.0f),
            nameFontSize,
            RAYWHITE
        );

        const int rightNameWidth = MeasureText(rightPlayer.username.c_str(), nameFontSize);
        DrawText(
            rightPlayer.username.c_str(),
            static_cast<int>(rightAvatar.x - static_cast<float>(rightNameWidth) * 0.5f),
            static_cast<int>(rightAvatar.y - avatarRadius - nameFontSize - 6.0f),
            nameFontSize,
            RAYWHITE
        );

        const Rectangle leftBox{
            arena.x + arena.width * 0.15f,
            arena.y + arena.height * 0.36f,
            arena.width * 0.25f,
            arena.height * 0.44f
        };

        const Rectangle rightBox{
            arena.x + arena.width * 0.60f,
            arena.y + arena.height * 0.36f,
            arena.width * 0.25f,
            arena.height * 0.44f
        };

        drawWeaponListBox(leftPlayer, leftBox);
        drawWeaponListBox(rightPlayer, rightBox);
    }

    void DuelManager::drawWeaponListBox(const Player& player, const Rectangle& box) const
    {
        DrawRectangleRounded(box, 0.18f, 12, Color{0, 0, 0, 80});
        DrawRectangleRoundedLinesEx(box, 0.18f, 12, 2.0f, RAYWHITE);

        const std::vector<std::string> lines = buildWeaponList(player);

        const int fontSize = static_cast<int>(box.height * 0.13f);
        const int lineSpacing = fontSize + 5;
        const int totalTextHeight = static_cast<int>(lines.size()) * lineSpacing;

        int y = static_cast<int>(box.y + box.height * 0.5f - static_cast<float>(totalTextHeight) * 0.5f);

        for (const std::string& line : lines)
        {
            const int textWidth = MeasureText(line.c_str(), fontSize);
            const int x = static_cast<int>(box.x + box.width * 0.5f - static_cast<float>(textWidth) * 0.5f);

            DrawText(line.c_str(), x, y, fontSize, RAYWHITE);
            y += lineSpacing;
        }
    }

    std::vector<std::string> DuelManager::buildWeaponList(const Player& player) const
    {
        int batCount = 0;
        int laserCount = 0;
        int gunCount = 0;
        int basicCount = 0;

        for (const auto& weapon : player.weapons)
        {
            if (!weapon)
            {
                continue;
            }

            const std::string& id = weapon->getId();

            if (id == "bat_weapon")
            {
                ++batCount;
            }
            else if (id == "laser_weapon")
            {
                ++laserCount;
            }
            else if (id == "gun_weapon")
            {
                ++gunCount;
            }
            else if (id == "basic_weapon")
            {
                ++basicCount;
            }
        }

        std::vector<std::string> lines;

        if (batCount > 0)
        {
            lines.push_back(std::to_string(batCount) + "xBat");
        }

        if (laserCount > 0)
        {
            lines.push_back(std::to_string(laserCount) + "xLaser");
        }

        if (gunCount > 0)
        {
            lines.push_back(std::to_string(gunCount) + "xGun");
        }

        if (basicCount > 0)
        {
            lines.push_back(std::to_string(basicCount) + "xBasic Attack");
        }

        if (lines.empty())
        {
            lines.push_back("No weapons");
        }

        return lines;
    }
}
