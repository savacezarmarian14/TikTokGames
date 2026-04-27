#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include "entities/Player.hpp"
#include "physics/CircleCollisionResolver.hpp"
#include "physics/MotionIntegrator.hpp"
#include "physics/PhysicsConfig.hpp"
#include "physics/WallCollisionResolver.hpp"
#include "raylib.h"

namespace TikTokArena
{
    class PhysicsSystem
    {
    public:
        explicit PhysicsSystem(PhysicsConfig config);
        bool update(Player& first, Player& second, const Rectangle& bounds, float deltaTime) const;

    private:
        MotionIntegrator motionIntegrator_;
        WallCollisionResolver wallCollisionResolver_;
        CircleCollisionResolver circleCollisionResolver_;
    };
}

#endif
