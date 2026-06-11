#include "systems/CombatSystem.hpp"
#include "utils/Math.hpp"
#include <algorithm>

namespace tw {

namespace {

sf::Vector2f gameplayUnitPosition(const Unit& unit,
                                  const std::map<int, sf::Vector2f>& towerPositions,
                                  float towerRadius,
                                  float unitRadius) {
    const auto sourceIt = towerPositions.find(unit.sourceTowerId());
    const auto targetIt = towerPositions.find(unit.targetTowerId());
    if (sourceIt == towerPositions.end() || targetIt == towerPositions.end()) {
        return {};
    }

    const auto source = sourceIt->second;
    const auto target = targetIt->second;

    const sf::Vector2f dir = math::normalize(target - source);

    const sf::Vector2f baseStart = source + dir * (towerRadius + unitRadius * 1.6f);
    const sf::Vector2f baseEnd = target - dir * (towerRadius + unitRadius * 1.6f);

    return math::lerp(baseStart, baseEnd, unit.progress());
}

sf::Vector2f gameplayTargetEdge(const Unit& unit,
                                const std::map<int, sf::Vector2f>& towerPositions,
                                float towerRadius,
                                float unitRadius) {
    const auto sourceIt = towerPositions.find(unit.sourceTowerId());
    const auto targetIt = towerPositions.find(unit.targetTowerId());
    if (sourceIt == towerPositions.end() || targetIt == towerPositions.end()) {
        return {};
    }

    const auto source = sourceIt->second;
    const auto target = targetIt->second;

    const sf::Vector2f dir = math::normalize(target - source);
    return target - dir * (towerRadius + unitRadius * 1.6f);
}

bool hasTowerPositions(const Unit& unit, const std::map<int, sf::Vector2f>& towerPositions) {
    return towerPositions.find(unit.sourceTowerId()) != towerPositions.end()
        && towerPositions.find(unit.targetTowerId()) != towerPositions.end();
}

} // namespace

CombatSystem::CombatSystem(const Config& config) : config_(config) {}

void CombatSystem::update(GameState& state,
                          const std::map<int, sf::Vector2f>& towerPositions,
                          float towerRadius) const {
    auto& units = state.units();

    for (auto& unitPtr : units) {
        if (!unitPtr || !unitPtr->isAlive()) {
            continue;
        }

        Unit& unit = *unitPtr;
        auto* targetTower = state.findTowerById(unit.targetTowerId());
        if (!targetTower || !targetTower->isAlive()) {
            unit.destroy();
            continue;
        }
        if (!hasTowerPositions(unit, towerPositions)) {
            unit.destroy();
            continue;
        }

        const auto unitPos = gameplayUnitPosition(unit, towerPositions, towerRadius, config_.unitRadius);
        const auto hitPoint = gameplayTargetEdge(unit, towerPositions, towerRadius, config_.unitRadius);

        if (math::distance(unitPos, hitPoint) <= config_.unitRadius * 1.25f || unit.progress() >= 0.999f) {
            targetTower->takeDamage(unit.damage());
            state.events().emplace_back(EventType::TowerDamaged, targetTower->id(), targetTower->team(), unit.damage());

            if (!targetTower->isAlive()) {
                state.events().emplace_back(EventType::TowerDestroyed, targetTower->id(), targetTower->team(), 0);
            }

            unit.destroy();
        }
    }

    units.erase(
        std::remove_if(units.begin(), units.end(), [](const GameState::UnitPtr& unitPtr) {
            return !unitPtr || !unitPtr->isAlive();
        }),
        units.end()
    );
}

} // namespace tw
