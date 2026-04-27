#include "duel/Duel.hpp"

#include <utility>

namespace TikTokArena
{
    Duel::Duel(Player leftPlayer, Player rightPlayer, Rectangle arenaBounds)
        : leftPlayer_(std::move(leftPlayer)),
          rightPlayer_(std::move(rightPlayer)),
          arenaBounds_(arenaBounds)
    {
    }

    Player& Duel::getLeftPlayer()
    {
        return leftPlayer_;
    }

    Player& Duel::getRightPlayer()
    {
        return rightPlayer_;
    }

    const Player& Duel::getLeftPlayer() const
    {
        return leftPlayer_;
    }

    const Player& Duel::getRightPlayer() const
    {
        return rightPlayer_;
    }

    const Rectangle& Duel::getArenaBounds() const
    {
        return arenaBounds_;
    }

    DuelState Duel::getState() const
    {
        return state_;
    }

    void Duel::start()
    {
        state_ = DuelState::Running;
        elapsedTime_ = 0.0f;
    }

    void Duel::finish(const std::string& resultText)
    {
        state_ = DuelState::Finished;
        resultText_ = resultText;

        leftPlayer_.velocity = Vector2{0.0f, 0.0f};
        rightPlayer_.velocity = Vector2{0.0f, 0.0f};
    }

    void Duel::updateElapsedTime(float deltaTime)
    {
        if (state_ == DuelState::Running)
        {
            elapsedTime_ += deltaTime;
        }
    }

    float Duel::getElapsedTime() const
    {
        return elapsedTime_;
    }

    const std::string& Duel::getResultText() const
    {
        return resultText_;
    }
}
