#ifndef WALL_COLLISION_RESOLVER_HPP
#define WALL_COLLISION_RESOLVER_HPP

#include "entities/Player.hpp"
#include "raylib.h"

namespace TikTokArena
{
    class WallCollisionResolver
    {
    public:
        void resolve(Player& player, const Rectangle& bounds) const;
    };
}

#endif
