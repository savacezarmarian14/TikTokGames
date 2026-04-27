#ifndef MOTION_INTEGRATOR_HPP
#define MOTION_INTEGRATOR_HPP

#include "entities/Player.hpp"

namespace TikTokArena
{
    class MotionIntegrator
    {
    public:
        void integrate(Player& player, float deltaTime) const;
    };
}

#endif
