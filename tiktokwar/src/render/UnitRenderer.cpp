#include "render/UnitRenderer.hpp"
#include "utils/Math.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tw {

namespace {
sf::Color withAlpha(const sf::Color& color, sf::Uint8 alpha) {
    return sf::Color(color.r, color.g, color.b, alpha);
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

sf::Color UnitRenderer::teamColor(Team team) const {
    return paletteColor(static_cast<int>(team));
}

void UnitRenderer::spawnBurst(const sf::Vector2f& pos, const sf::Color& color, bool strong) const {
    const int count = strong ? 10 : 6;
    const float speedMin = strong ? 48.0f : 24.0f;
    const float speedMax = strong ? 110.0f : 68.0f;

    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) * 0.85f;
        const float speed = speedMin + (speedMax - speedMin) * (static_cast<float>(i) / std::max(1, count - 1));

        Particle p;
        p.position = pos;
        p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
        p.color = color;
        p.radius = strong ? 2.8f : 2.1f;
        p.lifetime = strong ? 0.32f : 0.24f;
        p.maxLifetime = p.lifetime;
        particles_.push_back(p);
    }
}

void UnitRenderer::render(sf::RenderTarget& target,
                          const GameState& state,
                          const std::map<int, sf::Vector2f>& towerPositions,
                          float towerRadius,
                          float unitRadius) const {
    const float dt = std::min(0.05f, frameClock_.restart().asSeconds());
    const float t = static_cast<float>(animationClock_.getElapsedTime().asSeconds());

    for (auto it = particles_.begin(); it != particles_.end();) {
        it->lifetime -= dt;
        it->position += it->velocity * dt;
        it->velocity *= 0.93f;

        if (it->lifetime <= 0.0f) {
            it = particles_.erase(it);
        } else {
            ++it;
        }
    }

    std::unordered_map<int, sf::Vector2f> currentPositions;

    for (const auto& unit : state.units()) {
        const auto sourceIt = towerPositions.find(unit.sourceTowerId());
        const auto destinationIt = towerPositions.find(unit.targetTowerId());
        if (sourceIt == towerPositions.end() || destinationIt == towerPositions.end()) {
            continue;
        }

        const auto source = sourceIt->second;
        const auto destination = destinationIt->second;

        const sf::Vector2f dir = math::normalize(destination - source);
        const sf::Vector2f normal(-dir.y, dir.x);

        const float laneBase = std::sin(static_cast<float>(unit.id()) * 1.618f) * (towerRadius * 0.10f);
        const float laneWave = 2.5f * std::sin(t * 4.0f + static_cast<float>(unit.id()) * 0.45f + unit.progress() * 8.0f);
        const float laneOffset = laneBase + laneWave;

        const sf::Vector2f baseStart = source + dir * (towerRadius + unitRadius * 1.6f);
        const sf::Vector2f baseEnd = destination - dir * (towerRadius + unitRadius * 1.6f);

        const sf::Vector2f pos = math::lerp(baseStart, baseEnd, unit.progress()) + normal * laneOffset;
        currentPositions[unit.id()] = pos;

        const auto color = unit.isHealing() ? sf::Color(95, 255, 150) : teamColor(unit.owner());

        if (previousPositions_.find(unit.id()) == previousPositions_.end()) {
            spawnBurst(baseStart, color, false);
        }

        const float pulse = 0.95f + 0.16f * std::sin(t * 8.0f + unit.progress() * 14.0f + static_cast<float>(unit.id()));
        const float radius = unitRadius * pulse;

        for (int trail = 1; trail <= 5; ++trail) {
            const float trailProgress = math::clamp(unit.progress() - 0.022f * static_cast<float>(trail), 0.0f, 1.0f);
            const float trailWave = 2.5f * std::sin(t * 4.0f + static_cast<float>(unit.id()) * 0.45f + trailProgress * 8.0f);
            const sf::Vector2f trailPos = math::lerp(baseStart, baseEnd, trailProgress) + normal * (laneBase + trailWave);

            const float trailRadius = unitRadius * (1.0f - trail * 0.10f);
            sf::CircleShape ghost(std::max(1.0f, trailRadius));
            ghost.setOrigin(ghost.getRadius(), ghost.getRadius());
            ghost.setPosition(trailPos);
            ghost.setFillColor(withAlpha(color, static_cast<sf::Uint8>(130 - trail * 20)));
            target.draw(ghost);
        }

        sf::CircleShape glow(radius * 1.55f);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(pos);
        glow.setFillColor(withAlpha(color, 42));
        target.draw(glow);

        sf::CircleShape body(radius);
        body.setOrigin(radius, radius);
        body.setPosition(pos);
        body.setFillColor(color);
        body.setOutlineThickness(1.5f);
        body.setOutlineColor(sf::Color(245, 248, 255));
        target.draw(body);

        sf::CircleShape core(radius * 0.42f);
        core.setOrigin(core.getRadius(), core.getRadius());
        core.setPosition(pos - dir * 1.5f);
        core.setFillColor(withAlpha(sf::Color::White, 150));
        target.draw(core);
    }

    for (const auto& [unitId, lastPos] : previousPositions_) {
        if (currentPositions.find(unitId) == currentPositions.end()) {
            spawnBurst(lastPos, sf::Color(255, 255, 255), true);
        }
    }

    previousPositions_ = std::move(currentPositions);

    for (const auto& p : particles_) {
        const float ratio = p.lifetime / p.maxLifetime;

        sf::CircleShape glow(p.radius * 2.0f);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(p.position);
        glow.setFillColor(withAlpha(p.color, static_cast<sf::Uint8>(48 * ratio)));
        target.draw(glow);

        sf::CircleShape dot(p.radius);
        dot.setOrigin(dot.getRadius(), dot.getRadius());
        dot.setPosition(p.position);
        dot.setFillColor(withAlpha(p.color, static_cast<sf::Uint8>(200 * ratio)));
        target.draw(dot);
    }
}

} // namespace tw
