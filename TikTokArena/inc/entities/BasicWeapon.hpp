#ifndef BASIC_WEAPON_HPP
#define BASIC_WEAPON_HPP

#include "entities/Weapon.hpp"

namespace TikTokArena
{
    class BasicWeapon final : public Weapon
    {
    public:
        BasicWeapon();

        float getPlayerCollisionDamage() const override;

    private:
        static constexpr float DefaultPlayerCollisionDamage = 10.0f;

        float playerCollisionDamage_{DefaultPlayerCollisionDamage};
    };
}

#endif
