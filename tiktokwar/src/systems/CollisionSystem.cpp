/**
 * @file CollisionSystem.cpp
 * @brief Detects and resolves unit collisions along opposing lanes.
 */

#include "systems/CollisionSystem.hpp"
#include <algorithm>
#include "utils/Math.hpp"

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

} // namespace

bool CollisionSystem::sameLaneOppositeDirection(const Unit& a, const Unit& b) const {
    return a.sourceTowerId() == b.targetTowerId() && a.targetTowerId() == b.sourceTowerId();
}

bool hasTowerPositions(const Unit& unit, const std::map<int, sf::Vector2f>& towerPositions) {
    return towerPositions.find(unit.sourceTowerId()) != towerPositions.end()
        && towerPositions.find(unit.targetTowerId()) != towerPositions.end();
}

void CollisionSystem::update(GameState& state,
                             const std::map<int, sf::Vector2f>& towerPositions,
                             float towerRadius,
                             float unitRadius) const {
    auto& units = state.units();

    for (std::size_t i = 0; i < units.size(); ++i) {
        if (!units[i] || !units[i]->isAlive()) {
            continue;
        }
        if (!hasTowerPositions(*units[i], towerPositions)) {
            units[i]->destroy();
            continue;
        }

        const auto posA = gameplayUnitPosition(*units[i], towerPositions, towerRadius, unitRadius);

        for (std::size_t j = i + 1; j < units.size(); ++j) {
            if (!units[j] || !units[j]->isAlive()) {
                continue;
            }
            if (units[i]->owner() == units[j]->owner()) {
                continue;
            }
            if (!sameLaneOppositeDirection(*units[i], *units[j])) {
                continue;
            }
            if (!hasTowerPositions(*units[j], towerPositions)) {
                units[j]->destroy();
                continue;
            }

            const auto posB = gameplayUnitPosition(*units[j], towerPositions, towerRadius, unitRadius);

            if (math::distance(posA, posB) <= unitRadius * 2.2f) {
                units[i]->takeDamage(1);
                units[j]->takeDamage(1);
                int destroyed = 0;
                if (!units[i]->isAlive()) {
                    ++destroyed;
                }
                if (!units[j]->isAlive()) {
                    ++destroyed;
                }
                if (destroyed > 0) {
                    state.events().emplace_back(EventType::UnitDestroyed, -1, Team::None, destroyed);
                }
            }
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
