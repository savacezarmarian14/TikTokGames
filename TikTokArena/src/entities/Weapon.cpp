#include "entities/Weapon.hpp"

#include <utility>

namespace TikTokArena
{
    Weapon::Weapon(std::string id, std::string name)
        : id_(std::move(id)),
          name_(std::move(name))
    {
    }

    const std::string& Weapon::getId() const { return id_; }
    const std::string& Weapon::getName() const { return name_; }

    void Weapon::update(Player&, Player&, float) {}
    void Weapon::draw(const Player&) const {}
    void Weapon::reset() {}

    float Weapon::getPlayerCollisionDamage() const { return 0.0f; }
}
