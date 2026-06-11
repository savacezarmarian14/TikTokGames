#include "systems/MovementSystem.hpp"
#include <algorithm>

namespace tw {

void MovementSystem::update(GameState& state, float dt) const {
    if (!state.isRoundActive()) {
        return;
    }

    for (auto& unitPtr : state.units()) {
        if (!unitPtr || !unitPtr->isAlive()) {
            continue;
        }

        const float next = std::min(1.0f, unitPtr->progress() + unitPtr->speed() * dt);
        unitPtr->setProgress(next);
    }
}

} // namespace tw