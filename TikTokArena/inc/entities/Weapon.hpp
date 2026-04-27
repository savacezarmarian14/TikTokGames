#ifndef WEAPON_HPP
#define WEAPON_HPP

#include "raylib.h"

#include <string>

namespace TikTokArena
{
    class Player;

    class Weapon
    {
    public:
        Weapon(std::string id, std::string name);
        virtual ~Weapon() = default;

        const std::string& getId() const;
        const std::string& getName() const;

        virtual void update(Player& owner, Player& opponent, float deltaTime);
        virtual void draw(const Player& owner) const;
        virtual void reset();

        virtual float getPlayerCollisionDamage() const;

    private:
        std::string id_;
        std::string name_;
    };
}

#endif
