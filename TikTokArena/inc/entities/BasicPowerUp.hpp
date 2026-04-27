#ifndef BASIC_POWER_UP_HPP
#define BASIC_POWER_UP_HPP

#include "entities/PowerUp.hpp"

namespace TikTokArena
{
    class BasicPowerUp final : public PowerUp
    {
    public:
        BasicPowerUp();
        void apply() override;
    };
}

#endif
