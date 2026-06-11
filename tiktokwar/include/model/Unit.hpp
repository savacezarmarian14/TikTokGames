#pragma once

#include <memory>
#include "core/Types.hpp"

namespace tw {

/**
 * @file Unit.hpp
 * @brief Defines the abstract base type for all attack units.
 *
 * Derived unit types are stored polymorphically so the game can support
 * normal, shielded, and spiked units with distinct health and damage.
 */

/**
 * @brief A moving attack unit between two towers.
 */
class Unit {
public:
    /** @brief Constructs an invalid placeholder unit. */
    Unit() = default;
    /**
     * @brief Constructs a live unit at the start of its lane.
     *
     * @param id Stable identifier for the unit.
     * @param owner Owning team.
     * @param sourceTowerId Id of the source tower.
     * @param targetTowerId Id of the target tower.
     * @param speed Lane progress speed per second.
     * @param damage Damage applied on impact.
     * @param health Unit durability before destruction.
     */
    Unit(int id,
         Team owner,
         int sourceTowerId,
         int targetTowerId,
         float speed,
         int damage,
         int health);

    virtual ~Unit() = default;

    /** @brief Returns the concrete unit archetype. */
    virtual UnitKind kind() const = 0;

    /** @brief Stable numeric identifier. */
    int id() const;
    /** @brief Owning team. */
    Team owner() const;
    /** @brief Id of the source tower. */
    int sourceTowerId() const;
    /** @brief Id of the target tower. */
    int targetTowerId() const;

    /** @brief Current progress along the lane. */
    float progress() const;
    /** @brief Updates normalized lane progress. */
    void setProgress(float value);

    /** @brief Lane movement speed. */
    float speed() const;
    /** @brief Damage applied to the destination tower. */
    int damage() const;

    /** @brief Current unit health. */
    int health() const;
    /** @brief Maximum health at spawn. */
    int maxHealth() const;

    /** @brief Whether this unit remains active in the simulation. */
    bool isAlive() const;
    /** @brief Applies damage to this unit and destroys it when health expires. */
    void takeDamage(int amount);
    /** @brief Removes this unit from simulation immediately. */
    void destroy();

protected:
    int id_ = -1;
    Team owner_ = Team::None;
    int sourceTowerId_ = -1;
    int targetTowerId_ = -1;
    float progress_ = 0.0f;
    float speed_ = 0.0f;
    int damage_ = 0;
    int health_ = 1;
    int maxHealth_ = 1;
    bool alive_ = true;
};

using UnitPtr = std::unique_ptr<Unit>;

} // namespace tw
