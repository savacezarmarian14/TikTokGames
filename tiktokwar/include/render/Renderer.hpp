#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <SFML/Graphics.hpp>

#include "core/Config.hpp"
#include "core/Types.hpp"
#include "model/GameState.hpp"
#include "render/TowerRenderer.hpp"
#include "render/UnitRenderer.hpp"
#include "render/HudRenderer.hpp"
#include "render/EffectsRenderer.hpp"
#include "render/ViewportScaler.hpp"

namespace tw {

/**
 * @brief Coordinates all visual layers for the game window.
 */
class Renderer {
public:
    /** @brief Stores a shared configuration reference. Fonts load lazily. */
    explicit Renderer(const Config& config);
    /** @brief Renders the complete current frame to a window. */
    void render(sf::RenderWindow& window, const GameState& state, Team viewerTeam, bool cleanView);

    /** @brief Returns the virtual canvas used for proportional rendering and hit testing. */
    sf::Vector2u virtualCanvasSize() const;
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

    struct AttackGiftVisual {
        int sourceTowerId = -1;
        int targetTowerId = -1;
        std::string name;
        std::string texturePath;
    };

    struct HealGiftVisual {
        int towerId = -1;
        std::string name;
        std::string texturePath;
    };

private:
    sf::FloatRect infoButtonRect(const sf::Vector2u& windowSize) const;

    void drawBackground(sf::RenderTarget& target,
                        const sf::Vector2u& windowSize,
                        const std::map<int, sf::Vector2f>& positions,
                        bool showGrid) const;

    void drawEdges(sf::RenderTarget& target,
                   const std::map<int, sf::Vector2f>& positions) const;

    void drawAttackGiftIcons(sf::RenderTarget& target,
                             const GameState& state,
                             const std::map<int, sf::Vector2f>& positions) const;
    void drawHealGiftIcons(sf::RenderTarget& target,
                           const GameState& state,
                           const std::map<int, sf::Vector2f>& positions) const;
    void drawGiftIcon(sf::RenderTarget& target,
                      const sf::Texture& texture,
                      const sf::Vector2f& center,
                      const sf::Color& accentColor,
                      float plateRadius,
                      float iconSize) const;

    void drawInfoButton(sf::RenderTarget& target, const sf::Vector2u& windowSize, bool hovered) const;
    void drawTooltip(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const;
    void drawTeamLabel(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const;
    void ensureFontLoaded() const;

    void ensureAmbientOrbs(const sf::Vector2u& windowSize,
                           const std::map<int, sf::Vector2f>& positions) const;
    void updateAmbientOrbs(float dt,
                           const sf::Vector2u& windowSize,
                           const std::map<int, sf::Vector2f>& positions) const;
    sf::Color nearestZoneColor(const sf::Vector2f& point,
                               const std::map<int, sf::Vector2f>& positions) const;

    void ensureGiftCatalogLoaded() const;
    const sf::Texture* textureForGift(const std::string& path) const;

    static sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t);

private:
    const Config& config_;
    ViewportScaler viewportScaler_;
    TowerRenderer towerRenderer_;
    UnitRenderer unitRenderer_;
    HudRenderer hudRenderer_;
    EffectsRenderer effectsRenderer_;
    mutable sf::Font font_;
    mutable bool fontLoadAttempted_ = false;
    mutable bool fontLoaded_ = false;

    mutable std::vector<AmbientOrb> ambientOrbs_;
    mutable sf::Clock ambientClock_;
    mutable bool ambientInitialized_ = false;

    mutable bool giftCatalogLoadAttempted_ = false;
    mutable std::vector<AttackGiftVisual> attackGiftVisuals_;
    mutable std::vector<HealGiftVisual> healGiftVisuals_;
    mutable std::unordered_map<std::string, sf::Texture> giftTextures_;
    mutable std::unordered_set<std::string> failedGiftTextures_;
};

} // namespace tw
