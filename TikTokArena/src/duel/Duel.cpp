#include "duel/Duel.hpp"

#include <utility>

namespace TikTokArena
{
    Duel::Duel(Player leftPlayer, Player rightPlayer, Rectangle arenaBounds)
        : leftPlayer_(std::move(leftPlayer)), rightPlayer_(std::move(rightPlayer)), arenaBounds_(arenaBounds)
    {
    }

    Player& Duel::getLeftPlayer() { return leftPlayer_; }
    Player& Duel::getRightPlayer() { return rightPlayer_; }
    const Player& Duel::getLeftPlayer() const { return leftPlayer_; }
    const Player& Duel::getRightPlayer() const { return rightPlayer_; }
    const Rectangle& Duel::getArenaBounds() const { return arenaBounds_; }

    DuelState Duel::getState() const { return state_; }

    void Duel::finish(const std::string& winnerName)
    {
        state_ = DuelState::Finished;
        winnerName_ = winnerName;

        leftPlayer_.velocity = Vector2{0.0f, 0.0f};
        rightPlayer_.velocity = Vector2{0.0f, 0.0f};
    }

    const std::string& Duel::getWinnerName() const { return winnerName_; }
}
