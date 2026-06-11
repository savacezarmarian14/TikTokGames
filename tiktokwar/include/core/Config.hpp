#pragma once

#include <cstddef>
#include <string>

namespace tw {

/**
 * @brief Runtime configuration for local JSON input produced by external bridges.
 */
struct NetworkInputConfig {
    /** @brief Enables the local UDP input listener. */
    bool enabled = true;
    /** @brief Local address used for binding the UDP listener. */
    std::string host = "127.0.0.1";
    /** @brief UDP port used by the local listener. */
    unsigned short port = 7000;
    /** @brief Maximum UDP datagram size accepted from the bridge. */
    std::size_t maxDatagramSize = 8192;
};

/**
 * @brief Tunable gameplay, rendering, and simulation values.
 */
struct Config {
    /** @brief Default window width used by fixed-size window contexts. */
    int windowWidth = 1000;
    /** @brief Default window height used by fixed-size window contexts. */
    int windowHeight = 700;
    /** @brief Maximum frames per second requested from SFML. */
    unsigned int fpsLimit = 60;

    /** @brief Starting and maximum health for each tower. */
    int towerMaxHp = 1000;

    /** @brief Unit progress per second along a tower-to-tower lane. */
    float unitSpeed = 0.42f;
    /** @brief Damage dealt by one attack unit on impact. */
    int unitDamage = 1;
    /** @brief Health restored by one heal command donation. */
    int healAmount = 1;

    /** @brief Tower collision and rendering radius in pixels. */
    float towerRadius = 48.0f;
    /** @brief Unit collision and rendering radius in pixels. */
    float unitRadius = 7.0f;

    /** @brief Global cap preventing unbounded active units. */
    int maxUnits = 320;

    /** @brief Local generic JSON input, normally fed by the TikTok listener bridge. */
    NetworkInputConfig networkInput;
};

} // namespace tw
