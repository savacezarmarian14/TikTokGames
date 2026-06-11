#include "model/GameState.hpp"

namespace tw {

std::vector<Tower>& GameState::towers() { return towers_; }
std::vector<GameState::UnitPtr>& GameState::units() { return units_; }
std::vector<GameEvent>& GameState::events() { return events_; }

const std::vector<Tower>& GameState::towers() const { return towers_; }
const std::vector<GameState::UnitPtr>& GameState::units() const { return units_; }
const std::vector<GameEvent>& GameState::events() const { return events_; }

void GameState::clearEvents() { events_.clear(); }
void GameState::clearUnits() { units_.clear(); }

Tower* GameState::findTowerById(int id) {
    for (auto& tower : towers_) {
        if (tower.id() == id) {
            return &tower;
        }
    }
    return nullptr;
}

const Tower* GameState::findTowerById(int id) const {
    for (const auto& tower : towers_) {
        if (tower.id() == id) {
            return &tower;
        }
    }
    return nullptr;
}

bool GameState::isRoundActive() const { return roundActive_; }
void GameState::setRoundActive(bool active) { roundActive_ = active; }
int GameState::winnerId() const { return winnerId_; }
void GameState::setWinnerId(int id) { winnerId_ = id; }

} // namespace tw
