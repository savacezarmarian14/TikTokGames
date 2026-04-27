#include "core/Game.hpp"

#include "TikTokArena.hpp"
#include "raylib.h"
#include "test/TestPlayerFactory.hpp"

#include <algorithm>
#include <cmath>
#include <string>

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
        createTestPlayers();
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
        }
    }

    void Game::handleInput()
    {
        if (IsKeyPressed(config_.inputConfig.startDuelsKey) && gameState_ != GameState::Countdown)
        {
            // Porneste countdown-ul; duelurile vor fi create la sfarsit
            gameState_ = GameState::Countdown;
            countdownTimer_ = 0.0f;
        }

        if (IsKeyPressed(config_.inputConfig.resetKey))
        {
            reset();
        }
    }

    void Game::updateCountdown(float deltaTime)
    {
        countdownTimer_ += deltaTime;

        const float totalDuration = CountdownStepDuration * CountdownSteps;

        if (countdownTimer_ >= totalDuration)
        {
            // Countdown terminat - porneste duelurile
            duelManager_.startDuels(waitingPlayers_, arenaGrid_.getArenas());
            gameState_ = GameState::Running;
            countdownTimer_ = 0.0f;
        }
    }

    void Game::renderCountdown() const
    {
        const float totalDuration = CountdownStepDuration * CountdownSteps;
        const float elapsed = countdownTimer_;

        // Care cifra e acum: 3, 2, 1
        const int stepIndex = static_cast<int>(elapsed / CountdownStepDuration);
        const int digit = CountdownSteps - stepIndex; // 3 -> 2 -> 1

        // Progresul in cadrul pasului curent (0.0 -> 1.0)
        const float stepProgress = (elapsed - stepIndex * CountdownStepDuration) / CountdownStepDuration;

        // Animatie: cifra apare mare si se micsoreaza (ease-out)
        // scale: 1.0 la start, 0.4 la final
        const float scale = 1.0f - stepProgress * 0.6f;

        // Opacitate: plina la start, dispare spre final
        const unsigned char alpha = static_cast<unsigned char>((1.0f - stepProgress * stepProgress) * 255.0f);

        const int screenW = windowManager_.getWidth();
        const int screenH = windowManager_.getHeight();

        // Overlay semi-transparent
        DrawRectangle(0, 0, screenW, screenH, Color{0, 0, 0, 120});

        const std::string text = std::to_string(digit);
        const int baseFontSize = static_cast<int>(screenH * 0.4f);
        const int fontSize = static_cast<int>(baseFontSize * scale);

        const int textWidth = MeasureText(text.c_str(), fontSize);
        const int x = screenW / 2 - textWidth / 2;
        const int y = screenH / 2 - fontSize / 2;

        // Umbra pentru depth
        DrawText(
            text.c_str(),
            x + 6, y + 6,
            fontSize,
            Color{0, 0, 0, static_cast<unsigned char>(alpha / 2)}
        );

        // Cifra principala
        DrawText(
            text.c_str(),
            x, y,
            fontSize,
            Color{255, 255, 255, alpha}
        );
    }

    void Game::reset()
    {
        duelManager_.clearDuels();
        waitingPlayers_.clear();
        gameState_ = GameState::Idle;
        countdownTimer_ = 0.0f;
        createTestPlayers();
    }

    void Game::createTestPlayers()
    {
        const float baseUnit = static_cast<float>(std::min(windowManager_.getWidth(), windowManager_.getHeight()));
        const float radius = baseUnit * config_.playerSpawnConfig.radiusRatio;
        waitingPlayers_ = TestPlayerFactory::createRandomPlayers(
            config_.playerSpawnConfig.testPlayerCount,
            radius,
            config_.playerSpawnConfig.initialHealth
        );
    }

    void Game::render()
    {
        BeginDrawing();
        ClearBackground(config_.clearColor);
        backgroundParticles_.draw();
        arenaGrid_.draw();
        duelManager_.draw();

        if (gameState_ == GameState::Countdown)
        {
            renderCountdown();
        }

        EndDrawing();
    }
}
