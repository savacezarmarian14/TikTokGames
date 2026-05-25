#pragma once

#include "core/Types.hpp"
#include "core/Constants.hpp"

namespace tw {

/**
 * @brief A normalized gameplay request produced by an input adapter.
 */
struct GameCommand {
    /** @brief Kind of action to apply. */
    CommandType type = CommandType::SpawnAttack;
    /** @brief Requesting team; validated systems prefer the source tower team. */
    Team team = Team::None;
    /** @brief Source tower id for spawned units. */
    int sourceTowerId = InvalidId;
    /** @brief Destination tower id for attacks or healing units. */
    int targetTowerId = InvalidId;
    /** @brief Number of units requested by the command. */
    int count = 1;
};

} // namespace tw
