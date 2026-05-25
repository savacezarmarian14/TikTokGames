#pragma once

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Detects when a round has ended and records the winner.
 */
class RoundSystem {
public:
    /** @brief Ends the round once zero or one towers remain alive. */
    void update(GameState& state) const;
};

} // namespace tw
