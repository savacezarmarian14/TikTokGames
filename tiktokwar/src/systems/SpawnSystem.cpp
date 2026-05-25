#include "systems/SpawnSystem.hpp"
#include "utils/IdGenerator.hpp"

namespace tw {

SpawnSystem::SpawnSystem(const Config& config) : config_(config) {}

void SpawnSystem::apply(GameState& state, const std::vector<GameCommand>& commands) {
    for (const auto& command : commands) {
        if (command.type == CommandType::ResetGame) {
            for (auto& tower : state.towers()) {
                tower.reset();
            }
            state.clearUnits();
            state.clearEvents();
            state.setRoundActive(true);
            state.setWinnerId(-1);
            continue;
        }

        if (!state.isRoundActive()) {
            continue;
        }

        const bool isAttack = command.type == CommandType::SpawnAttack;
        const bool isHeal = command.type == CommandType::SpawnHeal;
        if (!isAttack && !isHeal) {
            continue;
        }

        auto* source = state.findTowerById(command.sourceTowerId);
        auto* target = state.findTowerById(command.targetTowerId);
        if (!source || !target || !source->isAlive() || !target->isAlive()) {
            continue;
        }

        int spawned = 0;
        for (int i = 0; i < command.count; ++i) {
            if (static_cast<int>(state.units().size()) >= config_.maxUnits) {
                break;
            }

            state.units().emplace_back(
                IdGenerator::next(),
                source->team(),
                source->id(),
                target->id(),
                config_.unitSpeed,
                isHeal ? config_.healAmount : config_.unitDamage,
                isHeal
            );
            ++spawned;
        }

        if (spawned > 0) {
            state.events().emplace_back(
                EventType::UnitSpawned,
                source->id(),
                source->team(),
                spawned
            );
        }
    }
}

} // namespace tw
