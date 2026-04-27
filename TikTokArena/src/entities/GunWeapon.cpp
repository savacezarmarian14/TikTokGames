#include "entities/GunWeapon.hpp"

#include "entities/Player.hpp"

#include <algorithm>
#include <cmath>

namespace TikTokArena
{
    GunWeapon::GunWeapon()
        : RotatingWeapon("gun_weapon", "Gun")
    {
        rotationSpeed_ = 2.7f;
    }

    void GunWeapon::update(Player& owner, Player& opponent, float deltaTime)
    {
        const Rectangle veryLargeArena{
            -100000.0f,
            -100000.0f,
            200000.0f,
            200000.0f
        };

        updateWithArena(owner, opponent, veryLargeArena, deltaTime);
    }

    void GunWeapon::updateWithArena(
        Player& owner,
        Player& opponent,
        const Rectangle& arenaBounds,
        float deltaTime
    )
    {
        rotate(deltaTime);

        fireTimer_ += deltaTime;

        while (fireTimer_ >= FireInterval)
        {
            fireTimer_ -= FireInterval;
            fireBullet(owner);
        }

        updateBullets(opponent, arenaBounds, deltaTime);
    }

    void GunWeapon::draw(const Player& owner) const
    {
        const Vector2 baseCenter = getTriangleBaseCenter(owner);
        const Vector2 tip = getTriangleTip(owner);
        const float halfBase = owner.radius * baseWidthMultiplier_ * 0.5f;

        const Vector2 direction = getDirection();
        const Vector2 perpendicular{-direction.y, direction.x};

        const Vector2 baseLeft{
            baseCenter.x + perpendicular.x * halfBase,
            baseCenter.y + perpendicular.y * halfBase
        };

        const Vector2 baseRight{
            baseCenter.x - perpendicular.x * halfBase,
            baseCenter.y - perpendicular.y * halfBase
        };

        // Raylib expects triangle vertices in a consistent winding order.
        // Draw it with the safe order and also add outline/details so the gun is always visible.
        DrawTriangle(baseRight, baseLeft, tip, gunColor_);
        DrawTriangleLines(baseRight, baseLeft, tip, Color{20, 20, 28, 255});

        // Extra visible direction line + glowing tip, useful because the gun is small relative to the arena.
        DrawLineEx(baseCenter, tip, std::max(2.0f, owner.radius * 0.10f), Color{255, 255, 255, 210});
        DrawCircleV(tip, std::max(2.0f, owner.radius * 0.13f), Color{255, 230, 90, 255});

        const float bulletRadius = std::max(2.0f, owner.radius * bulletRadiusMultiplier_);

        for (const GunBullet& bullet : bullets_)
        {
            if (!bullet.active)
            {
                continue;
            }

            DrawCircleV(bullet.position, bulletRadius, bulletColor_);
            DrawCircleLines(
                static_cast<int>(bullet.position.x),
                static_cast<int>(bullet.position.y),
                bulletRadius,
                Color{255, 255, 255, 180}
            );
        }
    }

    void GunWeapon::reset()
    {
        RotatingWeapon::reset();
        bullets_.clear();
        fireTimer_ = 0.0f;
    }

    Vector2 GunWeapon::getStartPoint(const Player& owner) const
    {
        return getTriangleBaseCenter(owner);
    }

    Vector2 GunWeapon::getEndPoint(const Player& owner) const
    {
        return getTriangleTip(owner);
    }

    float GunWeapon::getHitDamage() const
    {
        return 0.0f;
    }

    float GunWeapon::getHalfThickness(const Player& owner) const
    {
        return owner.radius * 0.10f;
    }

    void GunWeapon::fireBullet(const Player& owner)
    {
        const Vector2 direction = getDirection();
        const Vector2 tip = getTriangleTip(owner);
        const float bulletSpeed = owner.radius * bulletSpeedMultiplier_;

        bullets_.push_back(
            GunBullet{
                tip,
                Vector2{direction.x * bulletSpeed, direction.y * bulletSpeed},
                true
            }
        );
    }

    void GunWeapon::updateBullets(Player& opponent, const Rectangle& arenaBounds, float deltaTime)
    {
        for (GunBullet& bullet : bullets_)
        {
            if (!bullet.active)
            {
                continue;
            }

            bullet.position.x += bullet.velocity.x * deltaTime;
            bullet.position.y += bullet.velocity.y * deltaTime;

            if (doesBulletHitPlayer(bullet, opponent))
            {
                opponent.applyDamage(BulletDamage);
                bullet.active = false;
                continue;
            }

            if (isBulletOutsideArena(bullet, arenaBounds))
            {
                bullet.active = false;
            }
        }

        bullets_.erase(
            std::remove_if(
                bullets_.begin(),
                bullets_.end(),
                [](const GunBullet& bullet)
                {
                    return !bullet.active;
                }
            ),
            bullets_.end()
        );
    }

    bool GunWeapon::isBulletOutsideArena(const GunBullet& bullet, const Rectangle& arenaBounds) const
    {
        return bullet.position.x < arenaBounds.x ||
               bullet.position.x > arenaBounds.x + arenaBounds.width ||
               bullet.position.y < arenaBounds.y ||
               bullet.position.y > arenaBounds.y + arenaBounds.height;
    }

    bool GunWeapon::doesBulletHitPlayer(const GunBullet& bullet, const Player& player) const
    {
        const float dx = bullet.position.x - player.position.x;
        const float dy = bullet.position.y - player.position.y;
        const float distanceSquared = dx * dx + dy * dy;
        const float hitRadius = player.radius;

        return distanceSquared <= hitRadius * hitRadius;
    }

    Vector2 GunWeapon::getDirection() const
    {
        return Vector2{
            std::cos(currentAngle()),
            std::sin(currentAngle())
        };
    }

    Vector2 GunWeapon::getTriangleBaseCenter(const Player& owner) const
    {
        const Vector2 direction = getDirection();
        const float distance = owner.radius * distanceFromCenterMultiplier_;

        return Vector2{
            owner.position.x + direction.x * distance,
            owner.position.y + direction.y * distance
        };
    }

    Vector2 GunWeapon::getTriangleTip(const Player& owner) const
    {
        const Vector2 direction = getDirection();
        const float distance = owner.radius * (distanceFromCenterMultiplier_ + lengthMultiplier_);

        return Vector2{
            owner.position.x + direction.x * distance,
            owner.position.y + direction.y * distance
        };
    }

    Vector2 GunWeapon::rotatePoint(Vector2 point, Vector2 center, float angle) const
    {
        const float translatedX = point.x - center.x;
        const float translatedY = point.y - center.y;
        const float cosAngle = std::cos(angle);
        const float sinAngle = std::sin(angle);

        return Vector2{
            center.x + translatedX * cosAngle - translatedY * sinAngle,
            center.y + translatedX * sinAngle + translatedY * cosAngle
        };
    }
}
