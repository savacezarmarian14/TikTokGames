#include "input/InputManager.hpp"

namespace tw {

InputManager::InputManager(const NetworkInputConfig& networkInputConfig)
    : keyboardAdapter_(),
      tiktokAdapter_(networkInputConfig),
      pending_() {}

void InputManager::handleEvent(const sf::Event& event, std::size_t towerCount) {
    auto keyboardCommands = keyboardAdapter_.handleEvent(event, towerCount);
    pending_.insert(pending_.end(), keyboardCommands.begin(), keyboardCommands.end());
}

void InputManager::poll(std::size_t towerCount) {
    auto tiktokCommands = tiktokAdapter_.poll(towerCount);
    pending_.insert(pending_.end(), tiktokCommands.begin(), tiktokCommands.end());
}

std::vector<GameCommand> InputManager::consumeCommands() {
    auto out = pending_;
    pending_.clear();
    return out;
}

} // namespace tw
