#pragma once

#include <memory>
#include <vector>
#include "model/Tower.hpp"
#include "model/Unit.hpp"
#include "model/GameEvent.hpp"

namespace tw {

/**
 * @brief Mutable world state shared by systems during a frame.
 */
class GameState {
public:
    using UnitPtr = std::unique_ptr<Unit>;

    /** @brief Mutable tower collection. */
    std::vector<Tower>& towers();
    /** @brief Mutable active unit collection. */
    std::vector<UnitPtr>& units();
    /** @brief Mutable frame event collection. */
    std::vector<GameEvent>& events();

    /** @brief Read-only tower collection. */
    const std::vector<Tower>& towers() const;
    /** @brief Read-only active unit collection. */
    const std::vector<UnitPtr>& units() const;
    /** @brief Read-only frame event collection. */
    const std::vector<GameEvent>& events() const;

    /** @brief Removes events emitted during the previous frame. */
    void clearEvents();
    /** @brief Removes all active units. */
    void clearUnits();

    /** @brief Finds a tower by id, returning nullptr when absent. */
    Tower* findTowerById(int id);
    /** @brief Finds a tower by id, returning nullptr when absent. */
    const Tower* findTowerById(int id) const;

    /** @brief Whether gameplay systems should keep simulating the round. */
    bool isRoundActive() const;
    /** @brief Enables or disables round simulation. */
    void setRoundActive(bool active);

    /** @brief Id of the winning tower, or InvalidId/-1 when unresolved. */
    int winnerId() const;
    /** @brief Sets the winning tower id. */
    void setWinnerId(int id);

private:
    std::vector<Tower> towers_;
    std::vector<UnitPtr> units_;
    std::vector<GameEvent> events_;
    bool roundActive_ = true;
    int winnerId_ = -1;
};

} // namespace tw
