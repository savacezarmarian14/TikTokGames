#ifndef CIRCLE_COLLISION_RESOLVER_HPP
#define CIRCLE_COLLISION_RESOLVER_HPP

#include "entities/Player.hpp"
#include "physics/PhysicsConfig.hpp"

namespace TikTokArena
{
    class CircleCollisionResolver
    {
    public:
        explicit CircleCollisionResolver(PhysicsConfig config);
        bool resolve(Player& first, Player& second) const;

    private:
        PhysicsConfig config_{};
    };
}

#endif
