#pragma once

#include <SFML/Window/Event.hpp>
#include <cstddef>
#include <vector>

#include "core/GameCommand.hpp"
#include "input/KeyboardAdapter.hpp"
#include "input/TikTokAdapter.hpp"

namespace tw {

/**
 * @brief Aggregates commands from all input adapters.
 */
class InputManager {
public:
    /** @brief Adds commands produced by a window event. */
    void handleEvent(const sf::Event& event, std::size_t towerCount);
    /** @brief Returns and clears pending commands. */
    std::vector<GameCommand> consumeCommands();

private:
    KeyboardAdapter keyboardAdapter_;
    TikTokAdapter tiktokAdapter_;
    std::vector<GameCommand> pending_;
};

} // namespace tw
