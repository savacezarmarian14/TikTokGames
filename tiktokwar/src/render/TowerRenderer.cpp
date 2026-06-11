/**
 * @file TowerRenderer.cpp
 * @brief Draws tower bodies, health rings, and cartoon cannon barrels for each lane.
 */

#include "render/TowerRenderer.hpp"
#include "render/CannonGeometry.hpp"
#include "utils/Math.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace tw {

namespace {
constexpr float kPi = 3.14159265358979323846f;

sf::VertexArray makeArc(const sf::Vector2f& center,
                        float radius,
                        float startAngle,
                        float endAngle,
                        sf::Color color,
                        unsigned int pointCount = 64) {
    sf::VertexArray arc(sf::LineStrip, pointCount + 1);

    for (unsigned int i = 0; i <= pointCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(pointCount);
        const float angle = startAngle + (endAngle - startAngle) * t;

        const float x = center.x + std::cos(angle) * radius;
        const float y = center.y + std::sin(angle) * radius;

        arc[i].position = {x, y};
        arc[i].color = color;
    }

    return arc;
}

sf::Color paletteColor(int index) {
    static const std::vector<sf::Color> palette = {
        sf::Color(255, 90, 110),
        sf::Color(90, 165, 255),
        sf::Color(155, 90, 255),
        sf::Color(70, 240, 140),
        sf::Color(255, 185, 70),
        sf::Color(80, 235, 230),
        sf::Color(255, 120, 210),
        sf::Color(190, 255, 110),
        sf::Color(255, 130, 130),
        sf::Color(130, 160, 255),
        sf::Color(120, 255, 180),
        sf::Color(220, 150, 255)
    };

    if (index < 0) {
        return sf::Color::White;
    }
    return palette[static_cast<std::size_t>(index) % palette.size()];
}

void drawHealthTicks(sf::RenderTarget& target,
                     const sf::Vector2f& center,
                     float radius,
                     float ratio,
                     const sf::Color& activeColor,
                     bool alive) {
    constexpr int kTicks = 36;
    const int activeTicks = alive ? static_cast<int>(std::ceil(std::max(0.0f, std::min(1.0f, ratio)) * kTicks)) : 0;

    for (int i = 0; i < kTicks; ++i) {
        const float angle = -kPi / 2.0f + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(kTicks);
        const sf::Vector2f tickPos(
            center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius
        );
        const bool active = i < activeTicks;

        sf::CircleShape tick(active ? 2.4f : 1.55f);
        tick.setOrigin(tick.getRadius(), tick.getRadius());
        tick.setPosition(tickPos);
        tick.setFillColor(active
            ? sf::Color(activeColor.r, activeColor.g, activeColor.b, 235)
            : sf::Color(82, 92, 125, alive ? 115 : 45));
        target.draw(tick);
    }
}

std::string formatTowerHp(int hp) {
    const int safeHp = std::max(0, hp);
    return std::to_string(safeHp);
}

void centerText(sf::Text& text, const sf::Vector2f& position) {
    const auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(position);
}
}

sf::Color TowerRenderer::teamColor(Team team, bool alive) const {
    if (!alive) return sf::Color(90, 92, 104);
    return paletteColor(static_cast<int>(team));
}

sf::Color TowerRenderer::hpColor(float ratio) const {
    if (ratio >= 0.999f) {
        return sf::Color::White;
    }
    if (ratio >= 0.5f) {
        return sf::Color(100, 255, 130);
    }
    if (ratio >= 0.25f) {
        return sf::Color(255, 210, 70);
    }
    return sf::Color(255, 80, 80);
}

void TowerRenderer::render(sf::RenderTarget& target,
                           const GameState& state,
                           const std::map<int, sf::Vector2f>& towerPositions,
                           float towerRadius,
                           const sf::Font& font) const {
    const float t = clock_.getElapsedTime().asSeconds();
    render_geometry::updateCannonDebugOffset(towerRadius);

    for (const auto& tower : state.towers()) {
        const auto posIt = towerPositions.find(tower.id());
        if (posIt == towerPositions.end()) {
            continue;
        }

        const sf::Vector2f pos = posIt->second;
        const sf::Color base = teamColor(tower.team(), tower.isAlive());
        const float ratio = tower.maxHp() > 0 ? static_cast<float>(tower.hp()) / static_cast<float>(tower.maxHp()) : 0.0f;

        const float pulse = tower.isAlive() ? std::sin(t * 2.8f + static_cast<float>(tower.id()) * 0.9f) : 0.0f;
        const float ringPulse = render_geometry::towerRingPulse(tower.isAlive(), t, tower.id());
        const float innerPulse = tower.isAlive() ? 1.2f * std::sin(t * 3.6f + static_cast<float>(tower.id()) * 0.7f) : 0.0f;

        sf::CircleShape aura(towerRadius + 16.0f + ringPulse);
        aura.setOrigin(aura.getRadius(), aura.getRadius());
        aura.setPosition(pos);
        aura.setFillColor(sf::Color(base.r, base.g, base.b, tower.isAlive() ? 28 : 12));
        target.draw(aura);

        sf::CircleShape outer(towerRadius + innerPulse * 0.35f);
        outer.setOrigin(outer.getRadius(), outer.getRadius());
        outer.setPosition(pos);
        outer.setFillColor(sf::Color(base.r / 2, base.g / 2, base.b / 2));
        outer.setOutlineThickness(2.0f);
        outer.setOutlineColor(sf::Color(base.r / 3, base.g / 3, base.b / 3));
        target.draw(outer);

        sf::CircleShape inner(towerRadius * 0.78f + innerPulse * 0.15f);
        inner.setOrigin(inner.getRadius(), inner.getRadius());
        inner.setPosition(pos);
        inner.setFillColor(base);
        target.draw(inner);

        sf::CircleShape highlight(towerRadius * 0.22f);
        highlight.setOrigin(highlight.getRadius(), highlight.getRadius());
        highlight.setPosition(pos.x - towerRadius * 0.22f, pos.y - towerRadius * 0.22f);
        highlight.setFillColor(sf::Color(255, 255, 255, tower.isAlive() ? 48 : 16));
        target.draw(highlight);

        // Draw cartoon cannon barrels for each opponent lane.
        struct CannonMeta {
            sf::Vector2f direction;
        };
        std::vector<CannonMeta> cannonMeta;
        for (const auto& other : state.towers()) {
            if (other.id() == tower.id() || !other.isAlive()) {
                continue;
            }
            const auto otherPosIt = towerPositions.find(other.id());
            if (otherPosIt == towerPositions.end()) {
                continue;
            }
            const sf::Vector2f otherPos = otherPosIt->second;
            const sf::Vector2f direction = math::normalize(otherPos - pos);
            cannonMeta.push_back({direction});
        }

        for (std::size_t barrelIndex = 0; barrelIndex < cannonMeta.size(); ++barrelIndex) {
            const auto& meta = cannonMeta[barrelIndex];
            const float barrelThickness = render_geometry::cannonThickness(towerRadius);
            const float mountRadius = render_geometry::cannonMountRadius(towerRadius);
            const float barrelLength = render_geometry::cannonLength(towerRadius);
            const sf::Vector2f mountCenter = render_geometry::cannonMountCenter(pos, meta.direction, towerRadius, ringPulse);
            const sf::Vector2f barrelOrigin = render_geometry::cannonBarrelOrigin(pos, meta.direction, towerRadius, ringPulse);
            const sf::Color cannonColor = paletteColor(static_cast<int>(tower.team()));

            sf::CircleShape mount(mountRadius);
            mount.setOrigin(mount.getRadius(), mount.getRadius());
            mount.setPosition(mountCenter);
            mount.setFillColor(sf::Color(cannonColor.r / 2, cannonColor.g / 2, cannonColor.b / 2, 235));
            mount.setOutlineColor(sf::Color(245, 248, 255, 150));
            mount.setOutlineThickness(1.0f);
            target.draw(mount);

            sf::RectangleShape barrel({barrelLength, barrelThickness});
            barrel.setOrigin(0.0f, barrelThickness * 0.5f);
            barrel.setPosition(barrelOrigin);
            barrel.setRotation(std::atan2(meta.direction.y, meta.direction.x) * 180.0f / kPi);
            barrel.setFillColor(sf::Color(230, 245, 255, 230));
            barrel.setOutlineColor(sf::Color(cannonColor.r, cannonColor.g, cannonColor.b, 210));
            barrel.setOutlineThickness(1.0f);
            target.draw(barrel);

            sf::CircleShape muzzle(barrelThickness * 0.68f);
            muzzle.setOrigin(muzzle.getRadius(), muzzle.getRadius());
            muzzle.setPosition(barrelOrigin + meta.direction * barrelLength);
            muzzle.setFillColor(sf::Color(255, 255, 255, 235));
            muzzle.setOutlineColor(sf::Color(cannonColor.r, cannonColor.g, cannonColor.b, 230));
            muzzle.setOutlineThickness(1.2f);
            target.draw(muzzle);
        }

        const float hpRadius = render_geometry::hpRingRadius(towerRadius, ringPulse);

        sf::CircleShape hpHalo(hpRadius + 4.0f);
        hpHalo.setOrigin(hpHalo.getRadius(), hpHalo.getRadius());
        hpHalo.setPosition(pos);
        hpHalo.setFillColor(sf::Color::Transparent);
        hpHalo.setOutlineThickness(2.0f);
        hpHalo.setOutlineColor(sf::Color(base.r, base.g, base.b, tower.isAlive() ? 60 : 24));
        target.draw(hpHalo);

        sf::CircleShape hpBack(hpRadius);
        hpBack.setOrigin(hpBack.getRadius(), hpBack.getRadius());
        hpBack.setPosition(pos);
        hpBack.setFillColor(sf::Color::Transparent);
        hpBack.setOutlineThickness(6.0f);
        hpBack.setOutlineColor(sf::Color(30, 36, 56, 230));
        target.draw(hpBack);

        if (tower.isAlive() && ratio > 0.0f) {
            const float startAngle = -kPi / 2.0f;
            const float endAngle = startAngle + 2.0f * kPi * ratio;
            const auto glowArc = makeArc(pos, hpRadius + 2.0f, startAngle, endAngle, sf::Color(base.r, base.g, base.b, 110), 54);
            const auto arc = makeArc(pos, hpRadius, startAngle, endAngle, hpColor(ratio), 54);
            target.draw(glowArc);
            target.draw(arc);
        }

        drawHealthTicks(target, pos, hpRadius, ratio, hpColor(ratio), tower.isAlive());

        const std::string hpValue = tower.isAlive() ? formatTowerHp(tower.hp()) : "KO";
        const sf::Vector2f hpTextPos(pos.x, pos.y + towerRadius * 0.02f);

        sf::CircleShape hpPlate(towerRadius * 0.42f);
        hpPlate.setOrigin(hpPlate.getRadius(), hpPlate.getRadius());
        hpPlate.setPosition(hpTextPos);
        hpPlate.setScale(1.18f, 0.64f);
        hpPlate.setFillColor(sf::Color(5, 9, 20, tower.isAlive() ? 82 : 70));
        hpPlate.setOutlineThickness(1.0f);
        hpPlate.setOutlineColor(sf::Color(245, 248, 255, tower.isAlive() ? 58 : 38));
        target.draw(hpPlate);

        sf::Text hpShadow;
        hpShadow.setFont(font);
        hpShadow.setString(hpValue);
        hpShadow.setCharacterSize(20);
        hpShadow.setFillColor(sf::Color(4, 8, 18, 185));
        hpShadow.setStyle(sf::Text::Bold);
        centerText(hpShadow, hpTextPos + sf::Vector2f(1.2f, 1.6f));
        target.draw(hpShadow);

        sf::Text hpText;
        hpText.setFont(font);
        hpText.setString(hpValue);
        hpText.setCharacterSize(20);
        hpText.setFillColor(tower.isAlive() ? sf::Color::White : sf::Color(225, 230, 240));
        hpText.setOutlineThickness(1.0f);
        hpText.setOutlineColor(sf::Color(8, 12, 26, 165));
        hpText.setStyle(sf::Text::Bold);
        centerText(hpText, hpTextPos);
        target.draw(hpText);
    }
}

} // namespace tw
