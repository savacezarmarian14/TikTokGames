#ifndef DUEL_HPP
#define DUEL_HPP

#include "entities/Player.hpp"
#include <string>
#include "raylib.h"

namespace TikTokArena
{
    enum class DuelState
    {
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
        void finish(const std::string& winnerName);
        const std::string& getWinnerName() const;

    private:
        Player leftPlayer_;
        Player rightPlayer_;
        Rectangle arenaBounds_{};
        DuelState state_{DuelState::Running};
        std::string winnerName_;
    };
}

#endif
