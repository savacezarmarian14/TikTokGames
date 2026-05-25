#include "render/EffectsRenderer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace tw {

namespace {
constexpr float kPi = 3.14159265358979323846f;

sf::Color withAlpha(const sf::Color& color, sf::Uint8 alpha) {
    return sf::Color(color.r, color.g, color.b, alpha);
}

sf::Uint8 fadeAlpha(float t) {
    const float clamped = std::max(0.0f, std::min(1.0f, t));
    return static_cast<sf::Uint8>(255.0f * clamped);
}

float clamp01(float x) {
    return std::max(0.0f, std::min(1.0f, x));
}

float smoothOut(float t) {
    t = clamp01(t);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

} // namespace

EffectsRenderer::EffectsRenderer()
    : rng_(static_cast<unsigned int>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count())) {
    floatingTexts_.reserve(maxFloatingTexts_);
}

void EffectsRenderer::spawnFloatingText(const sf::Vector2f& center,
                                        float towerRadius,
                                        const std::string& text,
                                        const sf::Color& fillColor,
                                        const sf::Color& outlineColor,
                                        const sf::Vector2f& baseVelocity,
                                        float lifetime,
                                        unsigned int size,
                                        float startScale,
                                        float endScale) const {
    if (floatingTexts_.size() >= maxFloatingTexts_) {
        floatingTexts_.erase(floatingTexts_.begin());
    }

    std::uniform_real_distribution<float> offsetX(-towerRadius * 0.18f, towerRadius * 0.18f);
    std::uniform_real_distribution<float> offsetY(-towerRadius * 0.10f, towerRadius * 0.06f);
    std::uniform_real_distribution<float> velX(-7.0f, 7.0f);
    std::uniform_real_distribution<float> velY(-4.0f, 3.0f);

    FloatingText txt;
    txt.position = center + sf::Vector2f(offsetX(rng_), -towerRadius * 0.24f + offsetY(rng_));
    txt.velocity = baseVelocity + sf::Vector2f(velX(rng_), velY(rng_));
    txt.fillColor = fillColor;
    txt.outlineColor = outlineColor;
    txt.text = text;
    txt.lifetime = lifetime;
    txt.maxLifetime = lifetime;
    txt.size = size;
    txt.startScale = startScale;
    txt.endScale = endScale;

    floatingTexts_.push_back(std::move(txt));
}

void EffectsRenderer::updateFloatingTexts(float dt) const {
    for (auto& txt : floatingTexts_) {
        txt.lifetime -= dt;
        txt.position += txt.velocity * dt;
        txt.velocity.y -= 10.0f * dt;
        txt.velocity.x *= 0.985f;
    }

    floatingTexts_.erase(
        std::remove_if(floatingTexts_.begin(), floatingTexts_.end(),
                       [](const FloatingText& txt) { return txt.lifetime <= 0.0f; }),
        floatingTexts_.end()
    );
}

void EffectsRenderer::drawFloatingTexts(sf::RenderTarget& target, const sf::Font& font) const {
    for (const auto& txtData : floatingTexts_) {
        const float ratio = txtData.lifetime / txtData.maxLifetime;
        const float eased = smoothOut(1.0f - ratio);
        const float scale = txtData.startScale + (txtData.endScale - txtData.startScale) * eased;
        const sf::Uint8 alpha = fadeAlpha(ratio);

        sf::Text txt;
        txt.setFont(font);
        txt.setString(txtData.text);
        txt.setCharacterSize(txtData.size);
        txt.setStyle(sf::Text::Bold);
        txt.setFillColor(withAlpha(txtData.fillColor, alpha));
        txt.setOutlineColor(withAlpha(txtData.outlineColor, static_cast<sf::Uint8>(alpha * 0.95f)));
        txt.setOutlineThickness(2.4f);
        txt.setScale(scale, scale);

        const auto bounds = txt.getLocalBounds();
        txt.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
        txt.setPosition(txtData.position);
        target.draw(txt);
    }
}

void EffectsRenderer::render(sf::RenderTarget& target,
                             const GameState& state,
                             const std::map<int, sf::Vector2f>& towerPositions,
                             float towerRadius,
                             const sf::Font& font) const {
    const float dt = std::min(0.05f, frameClock_.restart().asSeconds());
    const float t = static_cast<float>(animationClock_.getElapsedTime().asSeconds());

    for (auto it = damageFlash_.begin(); it != damageFlash_.end();) {
        it->second = std::max(0.0f, it->second - dt * 3.0f);
        if (it->second <= 0.001f) {
            it = damageFlash_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = healFlash_.begin(); it != healFlash_.end();) {
        it->second = std::max(0.0f, it->second - dt * 2.8f);
        if (it->second <= 0.001f) {
            it = healFlash_.erase(it);
        } else {
            ++it;
        }
    }

    for (const auto& event : state.events()) {
        if (event.towerId() < 0) {
            continue;
        }

        auto posIt = towerPositions.find(event.towerId());
        if (posIt == towerPositions.end()) {
            continue;
        }

        const sf::Vector2f center = posIt->second;

        if (event.type() == EventType::TowerDamaged) {
            damageFlash_[event.towerId()] = std::min(1.0f, damageFlash_[event.towerId()] + 0.9f);

            spawnFloatingText(
                center,
                towerRadius,
                "-" + std::to_string(event.value()),
                sf::Color(255, 88, 88),
                sf::Color(95, 10, 10),
                sf::Vector2f(0.0f, -36.0f),
                0.55f,
                24,
                1.00f,
                0.68f
            );
        } else if (event.type() == EventType::TowerHealed) {
            healFlash_[event.towerId()] = std::min(1.0f, healFlash_[event.towerId()] + 0.85f);

            spawnFloatingText(
                center,
                towerRadius,
                "+" + std::to_string(event.value()),
                sf::Color(80, 255, 140),
                sf::Color(8, 80, 30),
                sf::Vector2f(0.0f, -27.0f),
                0.62f,
                24,
                1.00f,
                0.74f
            );
        } else if (event.type() == EventType::TowerDestroyed) {
            damageFlash_[event.towerId()] = 1.0f;

            spawnFloatingText(
                center,
                towerRadius,
                "DOWN",
                sf::Color(255, 220, 120),
                sf::Color(90, 55, 10),
                sf::Vector2f(0.0f, -18.0f),
                0.95f,
                26,
                1.06f,
                0.84f
            );
        }
    }

    for (const auto& [towerId, intensity] : damageFlash_) {
        auto it = towerPositions.find(towerId);
        if (it == towerPositions.end()) {
            continue;
        }

        const sf::Vector2f center = it->second;
        const float pulse = 1.0f + 0.06f * std::sin(t * 20.0f);

        sf::CircleShape glow((towerRadius + 12.0f) * pulse);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(center);
        glow.setFillColor(sf::Color(255, 90, 90, static_cast<sf::Uint8>(55 * intensity)));
        target.draw(glow);

        sf::CircleShape ring(towerRadius + 18.0f + 8.0f * (1.0f - intensity));
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(4.0f);
        ring.setOutlineColor(sf::Color(255, 100, 100, static_cast<sf::Uint8>(160 * intensity)));
        target.draw(ring);
    }

    for (const auto& [towerId, intensity] : healFlash_) {
        auto it = towerPositions.find(towerId);
        if (it == towerPositions.end()) {
            continue;
        }

        const sf::Vector2f center = it->second;

        sf::CircleShape glow(towerRadius + 10.0f);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(center);
        glow.setFillColor(sf::Color(90, 255, 150, static_cast<sf::Uint8>(48 * intensity)));
        target.draw(glow);

        sf::CircleShape ring(towerRadius + 14.0f + 6.0f * (1.0f - intensity));
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(center);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(3.0f);
        ring.setOutlineColor(sf::Color(90, 255, 150, static_cast<sf::Uint8>(140 * intensity)));
        target.draw(ring);
    }

    updateFloatingTexts(dt);
    drawFloatingTexts(target, font);
}

} // namespace tw
