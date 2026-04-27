#ifndef PHYSICS_CONFIG_HPP
#define PHYSICS_CONFIG_HPP

namespace TikTokArena
{
    struct PhysicsConfig
    {
        float minCollisionDistance;
        float collisionSeparationBias;
        float collisionRestitution;
    };
}

#endif