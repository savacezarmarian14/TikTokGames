#pragma once

#include <string>
#include "core/Types.hpp"

namespace tw {

/**
 * @brief A stationary base owned by a team.
 */
class Tower {
public:
    /** @brief Constructs an invalid placeholder tower. */
    Tower() = default;
    /** @brief Constructs a tower with full health. */
    Tower(int id, Team team, const std::string& name, int maxHp);

    /** @brief Stable numeric identifier. */
    int id() const;
    /** @brief Owning team. */
    Team team() const;
    /** @brief Display name. */
    const std::string& name() const;

    /** @brief Current health points. */
    int hp() const;
    /** @brief Maximum health points. */
    int maxHp() const;
    /** @brief Whether the tower can still spawn, heal, or be targeted. */
    bool isAlive() const;

    /** @brief Reduces health and marks the tower dead at zero. */
    void takeDamage(int amount);
    /** @brief Restores health without exceeding maxHp(). */
    void heal(int amount);
    /** @brief Restores the tower to its initial alive state. */
    void reset();

private:
    int id_ = -1;
    Team team_ = Team::None;
    std::string name_;
    int hp_ = 0;
    int maxHp_ = 0;
    bool alive_ = true;
};

} // namespace tw
