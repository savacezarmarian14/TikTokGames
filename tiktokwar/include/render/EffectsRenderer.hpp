#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <map>
#include <random>
#include <unordered_map>
#include <vector>

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Draws event-driven effects such as flashes and floating text.
 */
class EffectsRenderer {
public:
    /** @brief Creates deterministic storage and randomized effect state. */
    EffectsRenderer();

    /** @brief Processes current frame events and renders active effects. */
    void render(sf::RenderTarget& target,
                const GameState& state,
                const std::map<int, sf::Vector2f>& towerPositions,
                float towerRadius,
                const sf::Font& font) const;

private:
    struct FloatingText {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Color fillColor;
        sf::Color outlineColor;
        std::string text;
        float lifetime = 0.0f;
        float maxLifetime = 0.0f;
        float startScale = 1.0f;
        float endScale = 0.7f;
        unsigned int size = 22;
    };

private:
    /** @brief Adds one floating text effect near a tower. */
    void spawnFloatingText(const sf::Vector2f& center,
                           float towerRadius,
                           const std::string& text,
                           const sf::Color& fillColor,
                           const sf::Color& outlineColor,
                           const sf::Vector2f& baseVelocity,
                           float lifetime,
                           unsigned int size,
                           float startScale,
                           float endScale) const;

    /** @brief Advances and removes expired floating text effects. */
    void updateFloatingTexts(float dt) const;
    /** @brief Draws currently active floating text effects. */
    void drawFloatingTexts(sf::RenderTarget& target, const sf::Font& font) const;

private:
    mutable sf::Clock frameClock_;
    mutable sf::Clock animationClock_;
    mutable std::vector<FloatingText> floatingTexts_;
    mutable std::unordered_map<int, float> damageFlash_;
    mutable std::unordered_map<int, float> healFlash_;
    mutable std::mt19937 rng_;

    std::size_t maxFloatingTexts_ = 140;
};

} // namespace tw
