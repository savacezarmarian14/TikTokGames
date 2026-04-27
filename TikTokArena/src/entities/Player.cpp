#include "entities/Player.hpp"

#include <algorithm>
#include <utility>

namespace TikTokArena
{
    Player::Player(int id, std::string username, std::string photoPath, Color color, float radius, float maxHealth)
        : id(id),
          username(std::move(username)),
          photoPath(std::move(photoPath)),
          color(color),
          radius(radius),
          health(maxHealth),
          maxHealth(maxHealth)
    {
    }

    void Player::applyDamage(float damage)
    {
        health = std::max(0.0f, health - damage);
    }

    bool Player::isAlive() const
    {
        return health > 0.0f;
    }
}
