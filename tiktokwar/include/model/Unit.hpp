#pragma once

#include "core/Types.hpp"

namespace tw {

/**
 * @brief A moving attack or healing projectile between two towers.
 */
class Unit {
public:
    /** @brief Constructs an invalid placeholder unit. */
    Unit() = default;
    /** @brief Constructs a live unit at the start of its lane. */
    Unit(int id, Team owner, int sourceTowerId, int targetTowerId, float speed, int damage, bool healing);

    /** @brief Stable numeric identifier. */
    int id() const;
    /** @brief Owning team, used for colors and collision allegiance. */
    Team owner() const;
    /** @brief Id of the tower that spawned this unit. */
    int sourceTowerId() const;
    /** @brief Id of the tower this unit is moving toward. */
    int targetTowerId() const;

    /** @brief Normalized lane progress in the range [0, 1]. */
    float progress() const;
    /** @brief Updates normalized lane progress. */
    void setProgress(float value);

    /** @brief Progress added per second. */
    float speed() const;
    /** @brief Damage or healing amount applied on impact. */
    int damage() const;
    /** @brief True when this unit heals instead of damaging. */
    bool isHealing() const;

    /** @brief Whether the unit should remain in simulation. */
    bool isAlive() const;
    /** @brief Marks the unit for removal. */
    void destroy();

private:
    int id_ = -1;
    Team owner_ = Team::None;
    int sourceTowerId_ = -1;
    int targetTowerId_ = -1;
    float progress_ = 0.0f;
    float speed_ = 0.0f;
    int damage_ = 0;
    bool healing_ = false;
    bool alive_ = true;
};

} // namespace tw
