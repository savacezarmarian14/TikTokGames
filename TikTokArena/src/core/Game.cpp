#include "core/Game.hpp"

#include "TikTokArena.hpp"
#include "raylib.h"
#include "test/TestPlayerFactory.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace TikTokArena
{
    Game::Game(GameConfig config) : config_(config),
        backgroundParticles_(config_.backgroundParticlesStyle),
        arenaGrid_(config_.arenaRows, config_.arenaColumns, config_.arenaGridStyle),
        duelManager_(
            config_.physicsConfig,
            config_.playerSpawnConfig,
            config_.healthBarStyle
        )
    {
    }

    int Game::start()
    {
        initialize();
        run();
        return 0;
    }

    void Game::initialize()
    {
        windowManager_.createFullscreenWindow(GAME_NAME);
        backgroundParticles_.initialize(windowManager_.getWidth(), windowManager_.getHeight());
        arenaGrid_.updateLayout(windowManager_.getWidth(), windowManager_.getHeight());
    }

    void Game::run()
    {
        while (!windowManager_.shouldClose())
        {
            update();
            render();
        }
    }

    void Game::update()
    {
        const float deltaTime = GetFrameTime();

        handleInput();
        backgroundParticles_.update(deltaTime, windowManager_.getWidth(), windowManager_.getHeight());
        arenaGrid_.updateLayout(windowManager_.getWidth(), windowManager_.getHeight());

        if (gameState_ == GameState::Countdown)
        {
            updateCountdown(deltaTime);
        }
        else if (gameState_ == GameState::Running)
        {
            duelManager_.update(deltaTime);

            if (duelManager_.areAllDuelsFinished())
            {
                gameState_ = GameState::Results;
            }
        }
    }

    void Game::handleInput()
    {
        if (IsKeyPressed(config_.inputConfig.resetKey))
        {
            reset();
            return;
        }

        if (gameState_ != GameState::Idle)
        {
            return;
        }

        if (IsKeyPressed(KEY_ONE))
        {
            currentWeaponSelection_.addBat();
        }

        if (IsKeyPressed(KEY_TWO))
        {
            currentWeaponSelection_.addLaser();
        }

        if (IsKeyPressed(KEY_THREE))
        {
            currentWeaponSelection_.addGun();
        }

        if (IsKeyPressed(KEY_SPACE))
        {
            addWaitingPlayer();
        }

        if (IsKeyPressed(config_.inputConfig.startDuelsKey))
        {
            duelManager_.prepareDuels(waitingPlayers_, arenaGrid_.getArenas());

            if (duelManager_.hasActiveDuels())
            {
                currentWeaponSelection_.reset();
                gameState_ = GameState::Countdown;
                countdownTimer_ = 0.0f;
            }
        }
    }

    void Game::addWaitingPlayer()
    {
        const float baseUnit = static_cast<float>(std::min(windowManager_.getWidth(), windowManager_.getHeight()));
        const float radius = baseUnit * config_.playerSpawnConfig.radiusRatio;

        waitingPlayers_.push_back(
            TestPlayerFactory::createPlayerWithWeapons(
                nextPlayerId_,
                radius,
                config_.playerSpawnConfig.initialHealth,
                currentWeaponSelection_.getBatCount(),
                currentWeaponSelection_.getLaserCount(),
                currentWeaponSelection_.getGunCount()
            )
        );

        ++nextPlayerId_;
        currentWeaponSelection_.reset();
    }

    void Game::updateCountdown(float deltaTime)
    {
        countdownTimer_ += deltaTime;

        const float totalDuration = CountdownStepDuration * CountdownSteps;

        if (countdownTimer_ >= totalDuration)
        {
            duelManager_.activatePreparedDuels();
            gameState_ = GameState::Running;
            countdownTimer_ = 0.0f;
        }
    }

    void Game::renderCountdown() const
    {
        const float elapsed = countdownTimer_;

        const int stepIndex = static_cast<int>(elapsed / CountdownStepDuration);
        const int digit = std::max(1, CountdownSteps - stepIndex);

        const float stepProgress = (elapsed - stepIndex * CountdownStepDuration) / CountdownStepDuration;
        const float scale = 1.0f - stepProgress * 0.6f;
        const unsigned char alpha = static_cast<unsigned char>((1.0f - stepProgress * stepProgress) * 255.0f);

        const int screenW = windowManager_.getWidth();
        const int screenH = windowManager_.getHeight();

        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 90});

        const std::string text = std::to_string(digit);
        const int baseFontSize = static_cast<int>(screenH * 0.35f);
        const int fontSize = static_cast<int>(baseFontSize * scale);

        const int textWidth = MeasureText(text.c_str(), fontSize);
        const int x = screenW / 2 - textWidth / 2;
        const int y = screenH / 2 - fontSize / 2;

        DrawText(
            text.c_str(),
            x + 6, y + 6,
            fontSize,
            Color{0, 0, 0, static_cast<unsigned char>(alpha / 2)}
        );

        DrawText(
            text.c_str(),
            x, y,
            fontSize,
            Color{255, 255, 255, alpha}
        );
    }

    void Game::renderTop() const
    {
        const int screenW = windowManager_.getWidth();
        const int screenH = windowManager_.getHeight();
        const std::vector<WinnerResult> winners = duelManager_.getWinnerResultsSortedByTime();

        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 185});

        const char* title = "TOP WINNERS";
        const int titleFontSize = static_cast<int>(screenH * 0.07f);
        const int titleWidth = MeasureText(title, titleFontSize);

        DrawText(
            title,
            screenW / 2 - titleWidth / 2 + 5,
            static_cast<int>(screenH * 0.075f) + 5,
            titleFontSize,
            Color{0, 0, 0, 150}
        );

        DrawText(
            title,
            screenW / 2 - titleWidth / 2,
            static_cast<int>(screenH * 0.075f),
            titleFontSize,
            RAYWHITE
        );

        const Rectangle panel{
            screenW * 0.5f - screenW * 0.22f,
            screenH * 0.20f,
            screenW * 0.44f,
            screenH * 0.68f
        };

        DrawRectangleRounded(panel, 0.08f, 18, Color{18, 18, 28, 235});
        DrawRectangleRoundedLinesEx(panel, 0.08f, 18, 3.0f, Color{230, 230, 245, 210});

        if (winners.empty())
        {
            const char* noWinners = "NO WINNERS";
            const int fontSize = static_cast<int>(screenH * 0.045f);
            const int textWidth = MeasureText(noWinners, fontSize);

            DrawText(
                noWinners,
                screenW / 2 - textWidth / 2,
                static_cast<int>(panel.y + panel.height * 0.45f),
                fontSize,
                RAYWHITE
            );
            return;
        }

        const int visibleCount = std::min(static_cast<int>(winners.size()), 12);
        const float rowGap = panel.height * 0.018f;
        const float rowHeight = (panel.height * 0.86f - rowGap * static_cast<float>(visibleCount - 1)) /
                                static_cast<float>(visibleCount);

        float y = panel.y + panel.height * 0.07f;

        for (int index = 0; index < visibleCount; ++index)
        {
            const int rank = index + 1;
            const Color rankColor = getRankColor(rank);

            const Rectangle row{
                panel.x + panel.width * 0.07f,
                y,
                panel.width * 0.86f,
                rowHeight
            };

            DrawRectangleRounded(row, 0.20f, 14, Color{0, 0, 0, 95});
            DrawRectangleRoundedLinesEx(row, 0.20f, 14, rank <= 3 ? 3.0f : 1.5f, rankColor);

            if (rank <= 3)
            {
                drawTopRibbon(row, rank, rankColor);
            }

            const std::string rankText = std::to_string(rank) + ".";
            const int rankFontSize = static_cast<int>(row.height * 0.42f);

            DrawText(
                rankText.c_str(),
                static_cast<int>(row.x + row.width * 0.10f),
                static_cast<int>(row.y + row.height * 0.5f - rankFontSize * 0.5f),
                rankFontSize,
                rankColor
            );

            const std::string& playerName = winners[index].playerName;
            const int nameFontSize = static_cast<int>(row.height * 0.43f);
            const int nameWidth = MeasureText(playerName.c_str(), nameFontSize);

            DrawText(
                playerName.c_str(),
                static_cast<int>(row.x + row.width * 0.5f - nameWidth * 0.5f),
                static_cast<int>(row.y + row.height * 0.5f - nameFontSize * 0.5f),
                nameFontSize,
                rankColor
            );

            y += rowHeight + rowGap;
        }

        const char* hint = "Press R to reset";
        const int hintFontSize = static_cast<int>(screenH * 0.023f);
        const int hintWidth = MeasureText(hint, hintFontSize);

        DrawText(
            hint,
            screenW / 2 - hintWidth / 2,
            static_cast<int>(screenH * 0.91f),
            hintFontSize,
            Color{220, 220, 235, 255}
        );
    }

    void Game::drawTopRibbon(Rectangle row, int rank, Color color) const
    {
        const float centerX = row.x + row.width * 0.90f;
        const float centerY = row.y + row.height * 0.50f;
        const float medalRadius = row.height * 0.27f;

        DrawCircleV(Vector2{centerX, centerY}, medalRadius, color);
        DrawCircleLines(
            static_cast<int>(centerX),
            static_cast<int>(centerY),
            medalRadius,
            RAYWHITE
        );

        const std::string rankText = std::to_string(rank);
        const int fontSize = static_cast<int>(medalRadius * 1.05f);
        const int textWidth = MeasureText(rankText.c_str(), fontSize);

        DrawText(
            rankText.c_str(),
            static_cast<int>(centerX - textWidth * 0.5f),
            static_cast<int>(centerY - fontSize * 0.52f),
            fontSize,
            Color{20, 20, 25, 255}
        );

        const Vector2 leftRibbonA{centerX - medalRadius * 0.55f, centerY + medalRadius * 0.78f};
        const Vector2 leftRibbonB{centerX - medalRadius * 0.10f, centerY + medalRadius * 1.55f};
        const Vector2 leftRibbonC{centerX - medalRadius * 0.82f, centerY + medalRadius * 1.35f};

        const Vector2 rightRibbonA{centerX + medalRadius * 0.55f, centerY + medalRadius * 0.78f};
        const Vector2 rightRibbonB{centerX + medalRadius * 0.10f, centerY + medalRadius * 1.55f};
        const Vector2 rightRibbonC{centerX + medalRadius * 0.82f, centerY + medalRadius * 1.35f};

        DrawTriangle(leftRibbonA, leftRibbonB, leftRibbonC, Color{color.r, color.g, color.b, 170});
        DrawTriangle(rightRibbonA, rightRibbonC, rightRibbonB, Color{color.r, color.g, color.b, 170});
    }

    Color Game::getRankColor(int rank) const
    {
        if (rank == 1)
        {
            return Color{255, 215, 70, 255};
        }

        if (rank == 2)
        {
            return Color{200, 205, 215, 255};
        }

        if (rank == 3)
        {
            return Color{205, 127, 50, 255};
        }

        return RAYWHITE;
    }

    void Game::reset()
    {
        duelManager_.clearDuels();
        waitingPlayers_.clear();
        currentWeaponSelection_.reset();
        gameState_ = GameState::Idle;
        countdownTimer_ = 0.0f;
        nextPlayerId_ = 0;
    }

    void Game::render()
    {
        BeginDrawing();
        ClearBackground(config_.clearColor);

        backgroundParticles_.draw();
        arenaGrid_.draw();
        duelManager_.draw();

        if (gameState_ == GameState::Idle)
        {
            waitingPlayersRenderer_.draw(
                waitingPlayers_,
                currentWeaponSelection_,
                arenaGrid_.getArenas(),
                windowManager_.getWidth(),
                windowManager_.getHeight()
            );
        }

        if (gameState_ == GameState::Countdown)
        {
            renderCountdown();
        }

        if (gameState_ == GameState::Results)
        {
            renderTop();
        }

        EndDrawing();
    }
}
