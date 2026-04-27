#ifndef LASER_WEAPON_HPP
#define LASER_WEAPON_HPP

#include "entities/RotatingWeapon.hpp"

namespace TikTokArena
{
    class LaserWeapon final : public RotatingWeapon
    {
    public:
        LaserWeapon();

        void update(Player& owner, Player& opponent, float deltaTime) override;
        void draw(const Player& owner) const override;

    protected:
        Vector2 getStartPoint(const Player& owner) const override;
        Vector2 getEndPoint(const Player& owner) const override;
        float getHitDamage() const override { return damage_; }
        float getHalfThickness(const Player& owner) const override;

    private:
        float damage_{5.0f};
        float lengthMultiplier_{2.5f};        // mai lung decat bat (1.5)
        float thicknessMultiplier_{0.08f};    // subtire — e un laser
        float distanceFromCenterMultiplier_{1.25f};

        // Animatie: pulsatie de intensitate
        float pulseTimer_{0.0f};
        static constexpr float PulseSpeed = 8.0f;

        // Culori pentru animatia de glow
        static constexpr Color CoreColor  = Color{180, 230, 255, 255};
        static constexpr Color GlowColor  = Color{ 80, 160, 255,  80};
    };
}

#endif
