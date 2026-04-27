#include "entities/LaserWeapon.hpp"
#include "entities/Player.hpp"

#include <cmath>

namespace TikTokArena
{
    LaserWeapon::LaserWeapon()
        : RotatingWeapon("laser_weapon", "Laser Weapon")
    {
        rotationSpeed_ = 2.2f; // mai lent decat bat (3.2), mai impresionant
    }

    void LaserWeapon::update(Player& owner, Player& opponent, float deltaTime)
    {
        pulseTimer_ += deltaTime * PulseSpeed;
        if (pulseTimer_ >= 2.0f * PI)
            pulseTimer_ -= 2.0f * PI;

        // Logica de coliziune/damage din baza
        RotatingWeapon::update(owner, opponent, deltaTime);
    }

    Vector2 LaserWeapon::getStartPoint(const Player& owner) const
    {
        const float dist = owner.radius * distanceFromCenterMultiplier_;
        return Vector2{
            owner.position.x + std::cos(currentAngle()) * dist,
            owner.position.y + std::sin(currentAngle()) * dist
        };
    }

    Vector2 LaserWeapon::getEndPoint(const Player& owner) const
    {
        const float dist = owner.radius * distanceFromCenterMultiplier_;
        const float len  = owner.radius * lengthMultiplier_;
        return Vector2{
            owner.position.x + std::cos(currentAngle()) * (dist + len),
            owner.position.y + std::sin(currentAngle()) * (dist + len)
        };
    }

    float LaserWeapon::getHalfThickness(const Player& owner) const
    {
        return owner.radius * thicknessMultiplier_ * 0.5f;
    }

    void LaserWeapon::draw(const Player& owner) const
    {
        const Vector2 start = getStartPoint(owner);
        const Vector2 end   = getEndPoint(owner);

        // Pulsatie: sin intre 0 si 1
        const float pulse = (std::sin(pulseTimer_) + 1.0f) * 0.5f;

        // Glow exterior — mai gros, semi-transparent, pulseaza
        const float glowThickness = owner.radius * thicknessMultiplier_ * (3.0f + pulse * 2.0f);
        const unsigned char glowAlpha = static_cast<unsigned char>(40.0f + pulse * 60.0f);
        DrawLineEx(start, end, glowThickness, Color{GlowColor.r, GlowColor.g, GlowColor.b, glowAlpha});

        // Miezul laser-ului — subtire, alb-albastru, pulseaza usor in grosime
        const float coreThickness = owner.radius * thicknessMultiplier_ * (1.0f + pulse * 0.4f);
        DrawLineEx(start, end, coreThickness, CoreColor);

        // Punct stralucitor la varf
        const float tipRadius = owner.radius * thicknessMultiplier_ * (1.2f + pulse * 0.8f);
        const unsigned char tipAlpha = static_cast<unsigned char>(180.0f + pulse * 75.0f);
        DrawCircleV(end, tipRadius, Color{CoreColor.r, CoreColor.g, CoreColor.b, tipAlpha});
    }
}
