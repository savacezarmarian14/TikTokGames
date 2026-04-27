#ifndef ROTATING_WEAPON_HPP
#define ROTATING_WEAPON_HPP

#include "entities/Weapon.hpp"
#include "raylib.h"

namespace TikTokArena
{
    // Clasa de baza pentru orice weapon care se roteste in jurul playerului.
    // Gestioneaza unghiul, slot-ul si coliziunea cu inamicul (edge-triggered).
    class RotatingWeapon : public Weapon
    {
    public:
        RotatingWeapon(std::string id, std::string name);

        void setSlot(int slotIndex, int totalSlots);

        void update(Player& owner, Player& opponent, float deltaTime) override;
        void reset() override;

    protected:
        // Subclasele intorc start/end in coordonate absolute
        virtual Vector2 getStartPoint(const Player& owner) const = 0;
        virtual Vector2 getEndPoint(const Player& owner) const = 0;

        // Damage aplicat la o singura atingere
        virtual float getHitDamage() const = 0;

        // Grosimea capului weapon-ului (pentru detectia coliziunii)
        virtual float getHalfThickness(const Player& owner) const = 0;

        void rotate(float deltaTime);
        float currentAngle() const { return angle_ + slotAngleOffset_; }

        float rotationSpeed_{3.2f};

    private:
        bool intersectsPlayer(const Player& target, const Player& owner) const;
        float distancePointToSegment(Vector2 point, Vector2 start, Vector2 end) const;

        float angle_{0.0f};
        float slotAngleOffset_{0.0f};
        bool wasColliding_{false};
    };
}

#endif
