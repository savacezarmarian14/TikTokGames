#pragma once

#include <SFML/Window/Event.hpp>
#include <cstddef>
#include <vector>

#include "core/GameCommand.hpp"

namespace tw {

/**
 * @brief Translates SFML keyboard events into game commands.
 */
class KeyboardAdapter {
public:
    /** @brief Returns commands produced by one key event for the given tower count. */
    std::vector<GameCommand> handleEvent(const sf::Event& event, std::size_t towerCount) const;
};

} // namespace tw
