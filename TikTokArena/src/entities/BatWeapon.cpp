#include "entities/BatWeapon.hpp"
#include "entities/Player.hpp"

#include <cmath>

namespace TikTokArena
{
    BatWeapon::BatWeapon()
        : RotatingWeapon("bat_weapon", "Bat Weapon")
    {
    }

    Vector2 BatWeapon::getStartPoint(const Player& owner) const
    {
        const float dist = owner.radius * distanceFromCenterMultiplier_;
        return Vector2{
            owner.position.x + std::cos(currentAngle()) * dist,
            owner.position.y + std::sin(currentAngle()) * dist
        };
    }

    Vector2 BatWeapon::getEndPoint(const Player& owner) const
    {
        const float dist = owner.radius * distanceFromCenterMultiplier_;
        const float len  = owner.radius * lengthMultiplier_;
        return Vector2{
            owner.position.x + std::cos(currentAngle()) * (dist + len),
            owner.position.y + std::sin(currentAngle()) * (dist + len)
        };
    }

    float BatWeapon::getHalfThickness(const Player& owner) const
    {
        return owner.radius * thicknessMultiplier_ * 0.5f;
    }

    void BatWeapon::draw(const Player& owner) const
    {
        DrawLineEx(
            getStartPoint(owner),
            getEndPoint(owner),
            owner.radius * thicknessMultiplier_,
            color_
        );
    }
}
