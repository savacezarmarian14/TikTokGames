#include "entities/BasicWeapon.hpp"

namespace TikTokArena
{
    BasicWeapon::BasicWeapon()
        : Weapon("basic_weapon", "Basic Weapon")
    {
    }

    float BasicWeapon::getPlayerCollisionDamage() const
    {
        return playerCollisionDamage_;
    }
}