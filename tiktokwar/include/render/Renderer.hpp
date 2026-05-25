#pragma once

#include <map>
#include <vector>
#include <SFML/Graphics.hpp>

#include "core/Config.hpp"
#include "core/Types.hpp"
#include "model/GameState.hpp"
#include "render/TowerRenderer.hpp"
#include "render/UnitRenderer.hpp"
#include "render/HudRenderer.hpp"
#include "render/EffectsRenderer.hpp"

namespace tw {

/**
 * @brief Coordinates all visual layers for the game window.
 */
class Renderer {
public:
    /** @brief Stores a shared configuration reference and loads fonts. */
    explicit Renderer(const Config& config);
    /** @brief Renders the complete current frame to a window. */
    void render(sf::RenderWindow& window, const GameState& state, Team viewerTeam);

    /** @brief Computes tower positions for a circular arena layout. */
    std::map<int, sf::Vector2f> towerPositions(const sf::Vector2u& windowSize, std::size_t towerCount) const;
    /** @brief Returns configured tower radius. */
    float towerRadius() const;

private:
    struct AmbientOrb {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Color color;
        sf::Color targetColor;
        float radius = 2.0f;
        float phase = 0.0f;
    };

private:
    sf::FloatRect infoButtonRect(const sf::Vector2u& windowSize) const;

    void drawBackground(sf::RenderTarget& target,
                        const sf::Vector2u& windowSize,
                        const std::map<int, sf::Vector2f>& positions) const;

    void drawEdges(sf::RenderTarget& target,
                   const std::map<int, sf::Vector2f>& positions) const;

    void drawInfoButton(sf::RenderTarget& target, const sf::Vector2u& windowSize, bool hovered) const;
    void drawTooltip(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const;
    void drawTeamLabel(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const;

    void ensureAmbientOrbs(const sf::Vector2u& windowSize,
                           const std::map<int, sf::Vector2f>& positions) const;
    void updateAmbientOrbs(float dt,
                           const sf::Vector2u& windowSize,
                           const std::map<int, sf::Vector2f>& positions) const;
    sf::Color nearestZoneColor(const sf::Vector2f& point,
                               const std::map<int, sf::Vector2f>& positions) const;

    static sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t);

private:
    const Config& config_;
    TowerRenderer towerRenderer_;
    UnitRenderer unitRenderer_;
    HudRenderer hudRenderer_;
    EffectsRenderer effectsRenderer_;
    sf::Font font_;

    mutable std::vector<AmbientOrb> ambientOrbs_;
    mutable sf::Clock ambientClock_;
    mutable bool ambientInitialized_ = false;
};

} // namespace tw
