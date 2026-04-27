#include "physics/CircleCollisionResolver.hpp"

#include <cmath>

namespace TikTokArena
{
    CircleCollisionResolver::CircleCollisionResolver(PhysicsConfig config)
        : config_(config)
    {
    }

    bool CircleCollisionResolver::resolve(Player& first, Player& second) const
    {
        const float dx = second.position.x - first.position.x;
        const float dy = second.position.y - first.position.y;

        const float distanceSquared = dx * dx + dy * dy;
        const float minDistance = first.radius + second.radius;
        const float minDistanceSquared = minDistance * minDistance;

        if (distanceSquared >= minDistanceSquared)
        {
            return false;
        }

        float distance = std::sqrt(distanceSquared);

        if (distance <= config_.minCollisionDistance)
        {
            distance = minDistance;
        }

        const float nx = dx / distance;
        const float ny = dy / distance;

        const float overlap = minDistance - distance;
        const float separation = overlap * config_.collisionSeparationBias;

        first.position.x -= nx * separation;
        first.position.y -= ny * separation;

        second.position.x += nx * separation;
        second.position.y += ny * separation;

        const float dvx = second.velocity.x - first.velocity.x;
        const float dvy = second.velocity.y - first.velocity.y;

        const float velocityAlongNormal = dvx * nx + dvy * ny;

        if (velocityAlongNormal >= 0.0f)
        {
            return false;
        }

        const float restitution = config_.collisionRestitution;
        const float impulse = -(1.0f + restitution) * velocityAlongNormal / 2.0f;

        first.velocity.x -= impulse * nx;
        first.velocity.y -= impulse * ny;

        second.velocity.x += impulse * nx;
        second.velocity.y += impulse * ny;

        return true;
    }
}
