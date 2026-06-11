#pragma once

#include <map>
#include <utility>
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
    /**
     * @brief Stores a shared configuration reference.
     *
     * @param config Shared configuration used for unit speed, damage, healing,
     *               and behavior thresholds.
     */
    explicit SpawnSystem(const Config& config);
    /**
     * @brief Applies all pending commands to the current game state.
     *
     * Attack commands spawn units, while heal commands restore tower HP
     * immediately without creating healing units.
     */
    void apply(GameState& state, const std::vector<GameCommand>& commands);

private:
    using LaneKey = std::pair<int, int>;
    const Config& config_;
    std::map<LaneKey, int> donationStreaks_;
};

} // namespace tw
