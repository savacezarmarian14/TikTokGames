/**
 * @file SpawnSystem.cpp
 * @brief Builds units from game commands and awards special units for donation streaks.
 */

#include "model/NormalUnit.hpp"
#include "model/ShieldedUnit.hpp"
#include "model/SpikedUnit.hpp"
#include "systems/SpawnSystem.hpp"
#include "utils/IdGenerator.hpp"
#include <algorithm>

namespace tw {

namespace {

std::pair<int, int> makeLaneKey(int sourceId, int targetId) {
    return {sourceId, targetId};
}

} // namespace

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
            donationStreaks_.clear();
            continue;
        }

        if (!state.isRoundActive()) {
            continue;
        }

        if (command.type == CommandType::SpawnHeal) {
            auto* target = state.findTowerById(command.targetTowerId);
            if (!target || !target->isAlive()) {
                continue;
            }
            target->heal(command.count);
            state.events().emplace_back(EventType::TowerHealed,
                                       target->id(),
                                       target->team(),
                                       command.count);
            continue;
        }

        if (command.type != CommandType::SpawnAttack) {
            continue;
        }

        auto* source = state.findTowerById(command.sourceTowerId);
        auto* target = state.findTowerById(command.targetTowerId);
        if (!source || !target || !source->isAlive() || !target->isAlive()) {
            continue;
        }

        const int donationCount = std::max(0, command.count);
        if (donationCount <= 0) {
            continue;
        }

        const auto laneKey = makeLaneKey(source->id(), target->id());
        const int previousStreak = donationStreaks_[laneKey];

        auto spawnUnit = [&](GameState::UnitPtr unit) {
            if (static_cast<int>(state.units().size()) >= config_.maxUnits) {
                return false;
            }
            state.units().push_back(std::move(unit));
            return true;
        };

        int spawned = 0;
        auto placeLastSpawnedUnit = [&]() {
            constexpr float kWaveSpacing = 0.018f;
            constexpr float kMaxInitialProgress = 0.34f;
            state.units().back()->setProgress(
                std::min(kMaxInitialProgress, kWaveSpacing * static_cast<float>(spawned)));
            ++spawned;
        };

        for (int i = 0; i < donationCount; ++i) {
            const int streakSlot = (previousStreak + i + 1) % 10;
            GameState::UnitPtr unit;

            if (streakSlot == 0) {
                unit = std::make_unique<SpikedUnit>(
                    IdGenerator::next(),
                    source->team(),
                    source->id(),
                    target->id(),
                    config_.unitSpeed);
            } else if (streakSlot == 5) {
                unit = std::make_unique<ShieldedUnit>(
                    IdGenerator::next(),
                    source->team(),
                    source->id(),
                    target->id(),
                    config_.unitSpeed);
            } else {
                unit = std::make_unique<NormalUnit>(
                    IdGenerator::next(),
                    source->team(),
                    source->id(),
                    target->id(),
                    config_.unitSpeed);
            }

            if (spawnUnit(std::move(unit))) {
                placeLastSpawnedUnit();
            } else {
                break;
            }
        }

        donationStreaks_[laneKey] = (previousStreak + spawned) % 10;

        if (spawned > 0) {
            state.events().emplace_back(EventType::UnitSpawned,
                                       source->id(),
                                       source->team(),
                                       spawned);
        }
    }
}

} // namespace tw
