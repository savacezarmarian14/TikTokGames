#pragma once

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Advances live units along their lanes.
 */
class MovementSystem {
public:
    /** @brief Advances unit progress by speed * dt while the round is active. */
    void update(GameState& state, float dt) const;
};

} // namespace tw
