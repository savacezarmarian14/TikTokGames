#ifndef GAME_HPP
#define GAME_HPP

#include "core/GameConfig.hpp"
#include "duel/DuelManager.hpp"
#include "effects/BackgroundParticles.hpp"
#include "entities/Player.hpp"
#include "graphics/WindowManager.hpp"
#include "ui/ArenaGrid.hpp"

#include <vector>

namespace TikTokArena
{
    enum class GameState
    {
        Idle,
        Countdown,
        Running
    };

    class Game
    {
    public:
        explicit Game(GameConfig config = createDefaultGameConfig());
        int start();

    private:
        void initialize();
        void run();
        void update();
        void render();
        void handleInput();
        void createTestPlayers();
        void reset();

        void updateCountdown(float deltaTime);
        void renderCountdown() const;

    private:
        GameConfig config_;
        WindowManager windowManager_;
        BackgroundParticles backgroundParticles_;
        ArenaGrid arenaGrid_;
        DuelManager duelManager_;
        std::vector<Player> waitingPlayers_;

        GameState gameState_{GameState::Idle};
        float countdownTimer_{0.0f};
        static constexpr float CountdownStepDuration = 1.0f; // 1s per cifra
        static constexpr int CountdownSteps = 3;             // 3, 2, 1
    };
}

#endif
