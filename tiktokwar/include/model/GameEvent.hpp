#pragma once

#include "core/Types.hpp"
#include "core/Constants.hpp"

namespace tw {

/**
 * @brief Immutable event emitted by gameplay systems during a frame.
 */
class GameEvent {
public:
    /** @brief Constructs a default damage event for placeholder storage. */
    GameEvent() = default;
    /** @brief Constructs an event associated with a tower and team. */
    GameEvent(EventType type, int towerId, Team team, int value = 0);

    /** @brief Event category. */
    EventType type() const;
    /** @brief Related tower id, or InvalidId for global/unit-only events. */
    int towerId() const;
    /** @brief Related team, if any. */
    Team team() const;
    /** @brief Numeric payload such as damage, healing, or spawned count. */
    int value() const;

private:
    EventType type_ = EventType::TowerDamaged;
    int towerId_ = InvalidId;
    Team team_ = Team::None;
    int value_ = 0;
};

} // namespace tw
