#pragma once

namespace tw {

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
    int towerMaxHp = 200;

    /** @brief Unit progress per second along a tower-to-tower lane. */
    float unitSpeed = 0.42f;
    /** @brief Damage dealt by one attack unit on impact. */
    int unitDamage = 1;
    /** @brief Health restored by one healing unit on impact. */
    int healAmount = 1;

    /** @brief Tower collision and rendering radius in pixels. */
    float towerRadius = 48.0f;
    /** @brief Unit collision and rendering radius in pixels. */
    float unitRadius = 7.0f;

    /** @brief Global cap preventing unbounded active units. */
    int maxUnits = 500;
};

} // namespace tw
