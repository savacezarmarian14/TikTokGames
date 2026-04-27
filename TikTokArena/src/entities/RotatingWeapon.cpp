#include "entities/RotatingWeapon.hpp"
#include "entities/Player.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace TikTokArena
{
    RotatingWeapon::RotatingWeapon(std::string id, std::string name)
        : Weapon(std::move(id), std::move(name))
    {
    }

    void RotatingWeapon::setSlot(int slotIndex, int totalSlots)
    {
        if (totalSlots <= 0)
        {
            slotAngleOffset_ = 0.0f;
            return;
        }
        slotAngleOffset_ = static_cast<float>(slotIndex) * (2.0f * PI / static_cast<float>(totalSlots));
    }

    void RotatingWeapon::reset()
    {
        wasColliding_ = false;
        angle_ = 0.0f;
    }

    void RotatingWeapon::update(Player& owner, Player& opponent, float deltaTime)
    {
        rotate(deltaTime);

        const bool isColliding = intersectsPlayer(opponent, owner);

        if (isColliding && !wasColliding_)
            opponent.applyDamage(getHitDamage());

        wasColliding_ = isColliding;
    }

    void RotatingWeapon::rotate(float deltaTime)
    {
        angle_ += rotationSpeed_ * deltaTime;

        if (angle_ >= 2.0f * PI)
        {
            angle_ -= 2.0f * PI;
        }
    }

    bool RotatingWeapon::intersectsPlayer(const Player& target, const Player& owner) const
    {
        const Vector2 start = getStartPoint(owner);
        const Vector2 end = getEndPoint(owner);
        const float distance = distancePointToSegment(target.position, start, end);
        return distance <= target.radius + getHalfThickness(owner);
    }

    float RotatingWeapon::distancePointToSegment(Vector2 point, Vector2 start, Vector2 end) const
    {
        const float sx = end.x - start.x;
        const float sy = end.y - start.y;
        const float lenSq = sx * sx + sy * sy;

        if (lenSq <= 0.00001f)
        {
            const float dx = point.x - start.x;
            const float dy = point.y - start.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        const float t = std::clamp(
            ((point.x - start.x) * sx + (point.y - start.y) * sy) / lenSq,
            0.0f, 1.0f
        );

        const float px = start.x + t * sx;
        const float py = start.y + t * sy;
        const float dx = point.x - px;
        const float dy = point.y - py;
        return std::sqrt(dx * dx + dy * dy);
    }
}
