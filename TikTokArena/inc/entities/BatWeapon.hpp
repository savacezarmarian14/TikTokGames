#ifndef BAT_WEAPON_HPP
#define BAT_WEAPON_HPP

#include "entities/RotatingWeapon.hpp"

namespace TikTokArena
{
    class BatWeapon final : public RotatingWeapon
    {
    public:
        BatWeapon();

        void draw(const Player& owner) const override;

    protected:
        Vector2 getStartPoint(const Player& owner) const override;
        Vector2 getEndPoint(const Player& owner) const override;
        float getHitDamage() const override { return damage_; }
        float getHalfThickness(const Player& owner) const override;

    private:
        float damage_{2.0f};
        float lengthMultiplier_{1.5f};
        float thicknessMultiplier_{0.22f};
        float distanceFromCenterMultiplier_{1.25f};
        Color color_{220, 190, 120, 255};
    };
}

#endif
