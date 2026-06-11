#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace tw::render_geometry {

inline float cannonDebugExtraFactor = 0.06f;
inline bool wasTuneKeyDown = false;

inline void updateCannonDebugOffset(float towerRadius) {
    const bool tuneKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::F9);
    if (tuneKeyDown && !wasTuneKeyDown) {
        cannonDebugExtraFactor = std::min(0.28f, cannonDebugExtraFactor + 0.03f);
        std::cout << "[cannon-tune] extraFactor=" << cannonDebugExtraFactor
                  << " extraPixels=" << cannonDebugExtraFactor * towerRadius
                  << '\n';
    }
    wasTuneKeyDown = tuneKeyDown;
}

inline float towerRingPulse(bool alive, float t, int towerId) {
    return alive ? 2.2f * std::sin(t * 2.8f + static_cast<float>(towerId) * 0.9f) : 0.0f;
}

inline float hpRingRadius(float towerRadius, float ringPulse) {
    return towerRadius + 10.0f + ringPulse;
}

inline float cannonThickness(float towerRadius) {
    return std::max(5.5f, towerRadius * 0.15f);
}

inline float cannonMountRadius(float towerRadius) {
    return cannonThickness(towerRadius) * 1.15f;
}

inline float cannonLength(float towerRadius) {
    return std::max(towerRadius * 0.28f, cannonThickness(towerRadius) * 1.8f);
}

inline sf::Vector2f cannonMountCenter(const sf::Vector2f& towerCenter,
                                      const sf::Vector2f& direction,
                                      float towerRadius,
                                      float ringPulse) {
    const float distance = hpRingRadius(towerRadius, ringPulse)
        + cannonMountRadius(towerRadius)
        + towerRadius * cannonDebugExtraFactor;
    return towerCenter + direction * distance;
}

inline sf::Vector2f cannonBarrelOrigin(const sf::Vector2f& towerCenter,
                                       const sf::Vector2f& direction,
                                       float towerRadius,
                                       float ringPulse) {
    return cannonMountCenter(towerCenter, direction, towerRadius, ringPulse)
        + direction * (cannonMountRadius(towerRadius) * 0.25f);
}

inline sf::Vector2f cannonMuzzlePosition(const sf::Vector2f& towerCenter,
                                         const sf::Vector2f& direction,
                                         float towerRadius,
                                         float ringPulse) {
    return cannonBarrelOrigin(towerCenter, direction, towerRadius, ringPulse)
        + direction * cannonLength(towerRadius);
}

} // namespace tw::render_geometry
