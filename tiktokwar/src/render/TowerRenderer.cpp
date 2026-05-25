#include "render/TowerRenderer.hpp"
#include <cmath>
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
        sf::Color(70, 240, 140),
        sf::Color(155, 90, 255),
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

    for (const auto& tower : state.towers()) {
        const auto posIt = towerPositions.find(tower.id());
        if (posIt == towerPositions.end()) {
            continue;
        }

        const sf::Vector2f pos = posIt->second;
        const sf::Color base = teamColor(tower.team(), tower.isAlive());
        const float ratio = tower.maxHp() > 0 ? static_cast<float>(tower.hp()) / static_cast<float>(tower.maxHp()) : 0.0f;

        const float pulse = tower.isAlive() ? std::sin(t * 2.8f + static_cast<float>(tower.id()) * 0.9f) : 0.0f;
        const float ringPulse = tower.isAlive() ? 2.2f * pulse : 0.0f;
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

        sf::CircleShape hpBack(towerRadius + 10.0f + ringPulse);
        hpBack.setOrigin(hpBack.getRadius(), hpBack.getRadius());
        hpBack.setPosition(pos);
        hpBack.setFillColor(sf::Color::Transparent);
        hpBack.setOutlineThickness(5.0f);
        hpBack.setOutlineColor(sf::Color(50, 55, 70));
        target.draw(hpBack);

        if (tower.isAlive() && ratio > 0.0f) {
            const float arcRadius = towerRadius + 10.0f + ringPulse;
            const float startAngle = -kPi / 2.0f;
            const float endAngle = startAngle + 2.0f * kPi * ratio;
            const auto arc = makeArc(pos, arcRadius, startAngle, endAngle, hpColor(ratio), 90);
            target.draw(arc);
        }

        sf::Text hpText;
        hpText.setFont(font);
        hpText.setString(tower.isAlive() ? std::to_string(tower.hp()) : "X");
        hpText.setCharacterSize(24);
        hpText.setFillColor(sf::Color::White);
        hpText.setStyle(tower.isAlive() ? sf::Text::Bold : sf::Text::Regular);

        const auto hpBounds = hpText.getLocalBounds();
        hpText.setOrigin(hpBounds.left + hpBounds.width * 0.5f, hpBounds.top + hpBounds.height * 0.5f);
        hpText.setPosition(pos.x, pos.y);
        target.draw(hpText);
    }
}

} // namespace tw
