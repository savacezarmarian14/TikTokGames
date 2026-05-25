#pragma once

#include <vector>
#include "core/Config.hpp"
#include "core/GameCommand.hpp"
#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Converts high-level commands into units or round resets.
 */
class SpawnSystem {
public:
    /** @brief Stores a shared configuration reference. */
    explicit SpawnSystem(const Config& config);
    /** @brief Applies all pending commands to the current game state. */
    void apply(GameState& state, const std::vector<GameCommand>& commands);

private:
    const Config& config_;
};

} // namespace tw
