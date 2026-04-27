#include "physics/PhysicsSystem.hpp"

namespace TikTokArena
{
    PhysicsSystem::PhysicsSystem(PhysicsConfig config)
        : circleCollisionResolver_(config)
    {
    }

    bool PhysicsSystem::update(Player& first, Player& second, const Rectangle& bounds, float deltaTime) const
    {
        motionIntegrator_.integrate(first, deltaTime);
        motionIntegrator_.integrate(second, deltaTime);

        wallCollisionResolver_.resolve(first, bounds);
        wallCollisionResolver_.resolve(second, bounds);

        return circleCollisionResolver_.resolve(first, second);
    }
}
