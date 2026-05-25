#include "systems/RoundSystem.hpp"

namespace tw {

void RoundSystem::update(GameState& state) const {
    if (!state.isRoundActive()) {
        return;
    }

    int aliveCount = 0;
    int aliveId = -1;

    for (const auto& tower : state.towers()) {
        if (tower.isAlive()) {
            ++aliveCount;
            aliveId = tower.id();
        }
    }

    if (aliveCount <= 1) {
        state.setRoundActive(false);
        state.setWinnerId(aliveId);
        state.events().emplace_back(EventType::RoundEnded, aliveId, Team::None, 0);
    }
}

} // namespace tw
