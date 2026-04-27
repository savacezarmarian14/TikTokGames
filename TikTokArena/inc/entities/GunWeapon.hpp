#ifndef GUN_WEAPON_HPP
#define GUN_WEAPON_HPP

#include "entities/RotatingWeapon.hpp"
#include "raylib.h"

#include <vector>

namespace TikTokArena
{
    struct GunBullet
    {
        Vector2 position{};
        Vector2 velocity{};
        bool active{true};
    };

    class GunWeapon final : public RotatingWeapon
    {
    public:
        GunWeapon();

        void update(Player& owner, Player& opponent, float deltaTime) override;
        void updateWithArena(Player& owner, Player& opponent, const Rectangle& arenaBounds, float deltaTime);
        void draw(const Player& owner) const override;
        void reset() override;

    protected:
        Vector2 getStartPoint(const Player& owner) const override;
        Vector2 getEndPoint(const Player& owner) const override;
        float getHitDamage() const override;
        float getHalfThickness(const Player& owner) const override;

    private:
        void fireBullet(const Player& owner);
        void updateBullets(Player& opponent, const Rectangle& arenaBounds, float deltaTime);
        bool isBulletOutsideArena(const GunBullet& bullet, const Rectangle& arenaBounds) const;
        bool doesBulletHitPlayer(const GunBullet& bullet, const Player& player) const;

        Vector2 getDirection() const;
        Vector2 getTriangleBaseCenter(const Player& owner) const;
        Vector2 getTriangleTip(const Player& owner) const;
        Vector2 rotatePoint(Vector2 point, Vector2 center, float angle) const;

    private:
        std::vector<GunBullet> bullets_;

        float fireTimer_{0.0f};

        static constexpr float FireInterval = 0.5f; // 2 gloante / secunda
        static constexpr float BulletDamage = 30.0f;

        float bulletSpeedMultiplier_{5.0f};
        float bulletRadiusMultiplier_{0.14f};

        float distanceFromCenterMultiplier_{1.25f};
        float lengthMultiplier_{0.95f};
        float baseWidthMultiplier_{0.72f};

        Color gunColor_{235, 235, 245, 255};
        Color bulletColor_{255, 230, 90, 255};
    };
}

#endif
