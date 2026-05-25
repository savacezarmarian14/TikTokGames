#pragma once

#include <vector>
#include "core/GameCommand.hpp"

namespace tw {

/**
 * @brief Placeholder adapter for future TikTok live events.
 */
class TikTokAdapter {
public:
    /** @brief Polls integration commands; currently returns no commands. */
    std::vector<GameCommand> poll() const;
};

} // namespace tw
