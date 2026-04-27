#ifndef GAME_HPP
#define GAME_HPP

#include "core/GameConfig.hpp"
#include "duel/DuelManager.hpp"
#include "effects/BackgroundParticles.hpp"
#include "entities/Player.hpp"
#include "gameplay/WeaponSelection.hpp"
#include "graphics/WindowManager.hpp"
#include "ui/ArenaGrid.hpp"
#include "ui/WaitingPlayersRenderer.hpp"

#include <string>
#include <vector>

namespace TikTokArena
{
    enum class GameState
    {
        Idle,
        Countdown,
        Running,
        Results
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

        void addWaitingPlayer();
        void reset();

        void updateCountdown(float deltaTime);
        void renderCountdown() const;
        void renderTop() const;
        void drawTopRibbon(Rectangle panel, int rank, Color color) const;
        Color getRankColor(int rank) const;

    private:
        GameConfig config_;
        WindowManager windowManager_;
        BackgroundParticles backgroundParticles_;
        ArenaGrid arenaGrid_;
        DuelManager duelManager_;
        WaitingPlayersRenderer waitingPlayersRenderer_;

        std::vector<Player> waitingPlayers_;
        WeaponSelection currentWeaponSelection_;

        GameState gameState_{GameState::Idle};
        float countdownTimer_{0.0f};
        int nextPlayerId_{0};

        static constexpr float CountdownStepDuration = 1.0f;
        static constexpr int CountdownSteps = 3;
    };
}

#endif
