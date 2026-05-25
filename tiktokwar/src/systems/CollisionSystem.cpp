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
        if (!units[i].isAlive()) {
            continue;
        }
        if (!hasTowerPositions(units[i], towerPositions)) {
            units[i].destroy();
            continue;
        }

        const auto posA = gameplayUnitPosition(units[i], towerPositions, towerRadius, unitRadius);

        for (std::size_t j = i + 1; j < units.size(); ++j) {
            if (!units[j].isAlive()) {
                continue;
            }
            if (units[i].owner() == units[j].owner()) {
                continue;
            }
            if (!sameLaneOppositeDirection(units[i], units[j])) {
                continue;
            }
            if (!hasTowerPositions(units[j], towerPositions)) {
                units[j].destroy();
                continue;
            }

            const auto posB = gameplayUnitPosition(units[j], towerPositions, towerRadius, unitRadius);

            if (math::distance(posA, posB) <= unitRadius * 2.2f) {
                units[i].destroy();
                units[j].destroy();
                state.events().emplace_back(EventType::UnitDestroyed, -1, Team::None, 0);
            }
        }
    }

    units.erase(
        std::remove_if(units.begin(), units.end(), [](const Unit& unit) { return !unit.isAlive(); }),
        units.end()
    );
}

} // namespace tw
