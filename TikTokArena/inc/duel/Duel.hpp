#ifndef DUEL_HPP
#define DUEL_HPP

#include "entities/Player.hpp"
#include "raylib.h"

#include <string>

namespace TikTokArena
{
    enum class DuelState
    {
        Prepared,
        Running,
        Finished
    };

    class Duel
    {
    public:
        Duel(Player leftPlayer, Player rightPlayer, Rectangle arenaBounds);

        Player& getLeftPlayer();
        Player& getRightPlayer();
        const Player& getLeftPlayer() const;
        const Player& getRightPlayer() const;
        const Rectangle& getArenaBounds() const;

        DuelState getState() const;

        void start();
        void finish(const std::string& resultText);

        void updateElapsedTime(float deltaTime);
        float getElapsedTime() const;

        const std::string& getResultText() const;

    private:
        Player leftPlayer_;
        Player rightPlayer_;
        Rectangle arenaBounds_{};
        DuelState state_{DuelState::Prepared};
        std::string resultText_;
        float elapsedTime_{0.0f};
    };
}

#endif
