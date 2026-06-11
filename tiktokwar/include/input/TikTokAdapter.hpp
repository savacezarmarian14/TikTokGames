#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "core/Config.hpp"
#include "core/GameCommand.hpp"

namespace tw {

/**
 * @brief Metadata attached to a generic bridge event.
 *
 * The game does not depend on TikTok concepts. These fields are kept only for
 * logging/debugging incoming bridge events.
 */
struct ExternalInputMetadata {
    std::string eventId;
    std::string eventType;
    std::string username;
    std::string displayName;
    std::string giftName;
    int repeatCount = 1;
};

/**
 * @brief Command produced from a generic local JSON message.
 */
struct ExternalInputCommand {
    GameCommand command;
    ExternalInputMetadata metadata;
};

/**
 * @brief Receives generic game_event JSON messages from a local UDP bridge.
 *
 * The networking thread only parses and queues commands. Gameplay commands are
 * consumed later by the main game loop through poll().
 */
class TikTokAdapter {
public:
    explicit TikTokAdapter(NetworkInputConfig config = NetworkInputConfig{});
    ~TikTokAdapter();

    TikTokAdapter(const TikTokAdapter&) = delete;
    TikTokAdapter& operator=(const TikTokAdapter&) = delete;

    /** @brief Starts the UDP listener if enabled in the configuration. */
    void start();
    /** @brief Stops the UDP listener and joins its worker thread. */
    void stop();

    /** @brief Returns queued commands valid for the current tower count. */
    std::vector<GameCommand> poll(std::size_t towerCount);

private:
    void listenLoop();
    void enqueue(ExternalInputCommand command);

    NetworkInputConfig config_;
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::atomic<std::intptr_t> socketFd_{-1};

    std::mutex queueMutex_;
    std::queue<ExternalInputCommand> queue_;
};

} // namespace tw
