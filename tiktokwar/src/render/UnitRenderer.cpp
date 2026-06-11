/**
 * @file UnitRenderer.cpp
 * @brief Renders active moving units and their special visual effects.
 */

#include "render/UnitRenderer.hpp"
#include "render/CannonGeometry.hpp"
#include "utils/Math.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace tw {

namespace {
constexpr std::size_t kMaxParticles = 160;

sf::Color withAlpha(const sf::Color& color, sf::Uint8 alpha) {
    return sf::Color(color.r, color.g, color.b, alpha);
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
}

sf::Color UnitRenderer::teamColor(Team team) const {
    return paletteColor(static_cast<int>(team));
}

void UnitRenderer::spawnBurst(const sf::Vector2f& pos, const sf::Color& color, bool strong) const {
    if (particles_.size() >= kMaxParticles) {
        particles_.erase(particles_.begin(),
                         particles_.begin() + static_cast<std::ptrdiff_t>(particles_.size() - kMaxParticles + 1));
    }

    const int count = strong ? 6 : 3;
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
    const std::size_t unitCount = state.units().size();
    const bool heavyLoad = unitCount > 110;
    const bool extremeLoad = unitCount > 190;

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
    currentPositions.reserve(state.units().size());

    for (const auto& unitPtr : state.units()) {
        if (!unitPtr) {
            continue;
        }
        const Unit& unit = *unitPtr;
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
        const bool isShieldedUnit = unit.kind() == UnitKind::Shielded;
        const bool isSpikedUnit = unit.kind() == UnitKind::Spiked;
        const float laneWaveAmp = isSpikedUnit ? 4.0f : isShieldedUnit ? 3.0f : 2.0f;
        const float laneWave = laneWaveAmp * std::sin(t * 5.2f + static_cast<float>(unit.id()) * 0.45f + unit.progress() * 9.0f);
        const float laneOffset = laneBase + laneWave;

        const float sourceRingPulse = render_geometry::towerRingPulse(true, t, unit.sourceTowerId());
        const float targetRingPulse = render_geometry::towerRingPulse(true, t, unit.targetTowerId());
        const sf::Vector2f baseStart = render_geometry::cannonMuzzlePosition(source, dir, towerRadius, sourceRingPulse);
        const sf::Vector2f baseEnd = render_geometry::cannonMuzzlePosition(destination, -dir, towerRadius, targetRingPulse);

        const sf::Vector2f pos = math::lerp(baseStart, baseEnd, unit.progress()) + normal * laneOffset;
        currentPositions[unit.id()] = pos;

        const auto color = teamColor(unit.owner());
        const sf::Color laserColor = isSpikedUnit
            ? sf::Color(255, 235, 80)
            : isShieldedUnit ? sf::Color(120, 230, 255) : color;
        const float typeScale = isShieldedUnit ? 1.45f : isSpikedUnit ? 1.55f : 1.0f;

        if (!heavyLoad && previousPositions_.find(unit.id()) == previousPositions_.end()) {
            spawnBurst(baseStart, color, false);
        }

        const float pulse = 0.96f + 0.13f * std::sin(t * 8.5f + unit.progress() * 14.0f + static_cast<float>(unit.id()));
        const float radius = unitRadius * typeScale * pulse;
        const int trailCount = extremeLoad ? 0 : heavyLoad ? (isSpikedUnit ? 1 : 0) : isSpikedUnit ? 4 : 2;

        for (int trail = 1; trail <= trailCount; ++trail) {
            const float trailProgress = math::clamp(unit.progress() - 0.026f * static_cast<float>(trail), 0.0f, 1.0f);
            const float trailWave = laneWaveAmp * std::sin(t * 5.2f + static_cast<float>(unit.id()) * 0.45f + trailProgress * 9.0f);
            const sf::Vector2f trailPos = math::lerp(baseStart, baseEnd, trailProgress) + normal * (laneBase + trailWave);

            const float trailRadius = radius * (0.9f - trail * 0.12f);
            sf::CircleShape ghost(std::max(1.0f, trailRadius));
            ghost.setOrigin(ghost.getRadius(), ghost.getRadius());
            ghost.setPosition(trailPos);
            ghost.setFillColor(withAlpha(laserColor, static_cast<sf::Uint8>(120 - trail * 18)));
            target.draw(ghost);
        }

        if (!heavyLoad) {
            sf::CircleShape glow(radius * 1.55f);
            glow.setOrigin(glow.getRadius(), glow.getRadius());
            glow.setPosition(pos);
            glow.setFillColor(withAlpha(color, 42));
            target.draw(glow);
        }

        if (isSpikedUnit) {
            if (!heavyLoad) {
                sf::CircleShape hotGlow(radius * 2.35f);
                hotGlow.setOrigin(hotGlow.getRadius(), hotGlow.getRadius());
                hotGlow.setPosition(pos);
                hotGlow.setFillColor(withAlpha(sf::Color(255, 220, 70), 54));
                target.draw(hotGlow);
            }

            const float spin = t * 8.5f;
            const int spikeCount = extremeLoad ? 0 : heavyLoad ? 4 : 7;
            const float spikeRadius = radius * 1.15f;
            const float spikeLength = radius * 1.15f;
            for (int spike = 0; spike < spikeCount; ++spike) {
                const float angle = spin + static_cast<float>(spike) * (2.0f * 3.14159265f / static_cast<float>(spikeCount));
                const sf::Vector2f direction(std::cos(angle), std::sin(angle));
                const sf::Vector2f side(-direction.y, direction.x);
                const sf::Vector2f spikeBase = pos + direction * (spikeRadius - spikeLength * 0.42f);
                sf::ConvexShape tip;
                tip.setPointCount(3);
                tip.setPoint(0, spikeBase - side * (spikeLength * 0.24f));
                tip.setPoint(1, spikeBase + direction * spikeLength);
                tip.setPoint(2, spikeBase + side * (spikeLength * 0.24f));
                tip.setFillColor(withAlpha(sf::Color(255, 236, 75), 232));
                tip.setOutlineColor(withAlpha(sf::Color(255, 90, 40), 210));
                tip.setOutlineThickness(1.4f);
                target.draw(tip);
            }
        }

        sf::CircleShape body(radius);
        body.setOrigin(radius, radius);
        body.setPosition(pos);
        body.setFillColor(color);
        body.setOutlineThickness(1.5f);
        body.setOutlineColor(sf::Color(245, 248, 255));
        target.draw(body);

        if (isShieldedUnit) {
            if (!heavyLoad) {
                sf::CircleShape shieldGlow(radius * 2.05f);
                shieldGlow.setOrigin(shieldGlow.getRadius(), shieldGlow.getRadius());
                shieldGlow.setPosition(pos + dir * (radius * 0.58f));
                shieldGlow.setScale(0.82f, 1.15f);
                shieldGlow.setRotation(std::atan2(dir.y, dir.x) * 180.0f / 3.14159265f);
                shieldGlow.setFillColor(withAlpha(sf::Color(90, 220, 255), 58));
                target.draw(shieldGlow);
            }

            sf::ConvexShape shield;
            shield.setPointCount(7);
            const sf::Vector2f front = pos + dir * (radius * 1.45f);
            const sf::Vector2f back = pos - dir * (radius * 0.58f);
            const sf::Vector2f wide = normal * (radius * 1.0f);
            shield.setPoint(0, front);
            shield.setPoint(1, pos + dir * (radius * 0.92f) + wide * 0.62f);
            shield.setPoint(2, back + wide * 0.80f);
            shield.setPoint(3, back + wide * 0.25f);
            shield.setPoint(4, back - wide * 0.25f);
            shield.setPoint(5, back - wide * 0.80f);
            shield.setPoint(6, pos + dir * (radius * 0.92f) - wide * 0.62f);
            shield.setFillColor(sf::Color(105, 220, 255, 130));
            shield.setOutlineColor(sf::Color(230, 255, 255, 245));
            shield.setOutlineThickness(2.8f);
            target.draw(shield);
        }

        sf::CircleShape core(radius * 0.42f);
        core.setOrigin(core.getRadius(), core.getRadius());
        core.setPosition(pos - dir * 1.5f);
        core.setFillColor(withAlpha(sf::Color::White, 150));
        target.draw(core);
    }

    for (const auto& [unitId, lastPos] : previousPositions_) {
        if (!heavyLoad && currentPositions.find(unitId) == currentPositions.end()) {
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
