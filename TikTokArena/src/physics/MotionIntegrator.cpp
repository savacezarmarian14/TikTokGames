#include "physics/MotionIntegrator.hpp"

namespace TikTokArena
{
    void MotionIntegrator::integrate(Player& player, float deltaTime) const
    {
        player.position.x += player.velocity.x * deltaTime;
        player.position.y += player.velocity.y * deltaTime;
    }
}
