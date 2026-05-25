#include "systems/MovementSystem.hpp"
#include <algorithm>

namespace tw {

void MovementSystem::update(GameState& state, float dt) const {
    if (!state.isRoundActive()) {
        return;
    }

    for (auto& unit : state.units()) {
        if (!unit.isAlive()) {
            continue;
        }

        const float next = std::min(1.0f, unit.progress() + unit.speed() * dt);
        unit.setProgress(next);
    }
}

} // namespace tw