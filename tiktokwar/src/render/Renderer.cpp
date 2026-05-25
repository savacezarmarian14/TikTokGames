#include "render/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace tw {

namespace {

void drawPromoText(sf::RenderTarget& target,
                   const sf::Vector2u& windowSize,
                   const sf::Font& font) {
    static sf::Clock clock;
    const float t = clock.getElapsedTime().asSeconds();

    const std::string msg = "COMING LIVE FOR WAR SOON";

    sf::Text text;
    text.setFont(font);
    text.setString(msg);
    text.setCharacterSize(34);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(sf::Color(255, 255, 255, 24));
    text.setOutlineColor(sf::Color(0, 0, 0, 26));
    text.setOutlineThickness(2.0f);

    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f,
                   bounds.top + bounds.height * 0.5f);

    const float w = static_cast<float>(windowSize.x);
    const float h = static_cast<float>(windowSize.y);

    struct PromoSlot {
        float x;
        float y;
        float rotation;
    };

    const std::vector<PromoSlot> slots = {
        {w * 0.22f, h * 0.24f, -18.0f},
        {w * 0.78f, h * 0.28f, -18.0f},
        {w * 0.50f, h * 0.82f, -18.0f}
    };

    for (std::size_t i = 0; i < slots.size(); ++i) {
        const float driftX = std::sin(t * 0.55f + static_cast<float>(i) * 1.7f) * 8.0f;
        const float driftY = std::cos(t * 0.45f + static_cast<float>(i) * 1.3f) * 5.0f;

        text.setPosition(slots[i].x + driftX, slots[i].y + driftY);
        text.setRotation(slots[i].rotation);
        target.draw(text);
    }
}


constexpr float kPi = 3.14159265358979323846f;

sf::Clock gRenderClock;

float clamp01(float x) {
    return std::max(0.0f, std::min(1.0f, x));
}

float lengthSquared(const sf::Vector2f& v) {
    return v.x * v.x + v.y * v.y;
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

} // namespace

Renderer::Renderer(const Config& config) : config_(config) {
    if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")) {
        std::cerr << "Warning: failed to load default font. Text may not render correctly.\n";
    }
}

std::map<int, sf::Vector2f> Renderer::towerPositions(const sf::Vector2u& windowSize, std::size_t towerCount) const {
    std::map<int, sf::Vector2f> positions;
    if (towerCount == 0) {
        return positions;
    }

    const float w = static_cast<float>(windowSize.x);
    const float h = static_cast<float>(windowSize.y);

    const sf::Vector2f center(w * 0.5f, h * 0.5f + 18.0f);
    const float usableRadius = std::min(w, h) * 0.36f;
    const float startAngle = -kPi * 0.5f;

    for (std::size_t i = 0; i < towerCount; ++i) {
        const float angle = startAngle + 2.0f * kPi * static_cast<float>(i) / static_cast<float>(towerCount);
        positions[static_cast<int>(i)] = {
            center.x + std::cos(angle) * usableRadius,
            center.y + std::sin(angle) * usableRadius
        };
    }

    return positions;
}

float Renderer::towerRadius() const {
    return config_.towerRadius;
}

sf::FloatRect Renderer::infoButtonRect(const sf::Vector2u& windowSize) const {
    const float size = 34.0f;
    const float margin = 16.0f;
    return {static_cast<float>(windowSize.x) - size - margin, margin, size, size};
}

sf::Color Renderer::lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    t = clamp01(t);

    auto lerpChannel = [t](sf::Uint8 x, sf::Uint8 y) -> sf::Uint8 {
        const float value = static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t;
        return static_cast<sf::Uint8>(std::round(std::max(0.0f, std::min(255.0f, value))));
    };

    return sf::Color(
        lerpChannel(a.r, b.r),
        lerpChannel(a.g, b.g),
        lerpChannel(a.b, b.b),
        lerpChannel(a.a, b.a)
    );
}

sf::Color Renderer::nearestZoneColor(const sf::Vector2f& point,
                                     const std::map<int, sf::Vector2f>& positions) const {
    int bestId = -1;
    float bestDist = 0.0f;

    for (const auto& [id, pos] : positions) {
        const float d = lengthSquared(point - pos);
        if (bestId < 0 || d < bestDist) {
            bestId = id;
            bestDist = d;
        }
    }

    if (bestId < 0) {
        return sf::Color(180, 190, 220, 36);
    }

    const auto c = paletteColor(bestId);
    return sf::Color(c.r, c.g, c.b, 42);
}

void Renderer::ensureAmbientOrbs(const sf::Vector2u& windowSize,
                                 const std::map<int, sf::Vector2f>& positions) const {
    if (ambientInitialized_) {
        return;
    }

    ambientInitialized_ = true;
    ambientOrbs_.clear();

    std::mt19937 rng(static_cast<unsigned int>(std::random_device{}()));

    std::uniform_real_distribution<float> distX(0.0f, static_cast<float>(windowSize.x));
    std::uniform_real_distribution<float> distY(0.0f, static_cast<float>(windowSize.y));
    std::uniform_real_distribution<float> distVel(-14.0f, 14.0f);
    std::uniform_real_distribution<float> distRadius(2.0f, 6.5f);
    std::uniform_real_distribution<float> distPhase(0.0f, 6.28318530718f);

    constexpr int orbCount = 70;
    ambientOrbs_.reserve(orbCount);

    for (int i = 0; i < orbCount; ++i) {
        AmbientOrb orb;
        orb.position = {distX(rng), distY(rng)};
        orb.velocity = {distVel(rng), distVel(rng)};
        orb.radius = distRadius(rng);
        orb.phase = distPhase(rng);

        if (std::abs(orb.velocity.x) < 4.0f) {
            orb.velocity.x = (orb.velocity.x < 0.0f) ? -4.0f : 4.0f;
        }
        if (std::abs(orb.velocity.y) < 4.0f) {
            orb.velocity.y = (orb.velocity.y < 0.0f) ? -4.0f : 4.0f;
        }

        orb.targetColor = nearestZoneColor(orb.position, positions);
        orb.color = orb.targetColor;
        ambientOrbs_.push_back(orb);
    }

    ambientClock_.restart();
}

void Renderer::updateAmbientOrbs(float dt,
                                 const sf::Vector2u& windowSize,
                                 const std::map<int, sf::Vector2f>& positions) const {
    const float w = static_cast<float>(windowSize.x);
    const float h = static_cast<float>(windowSize.y);

    for (auto& orb : ambientOrbs_) {
        orb.phase += dt * 0.8f;

        const sf::Vector2f drift(
            std::cos(orb.phase * 0.9f) * 3.5f,
            std::sin(orb.phase * 1.1f) * 3.5f
        );

        orb.position += (orb.velocity + drift) * dt;

        if (orb.position.x < -20.0f) orb.position.x = w + 20.0f;
        if (orb.position.x > w + 20.0f) orb.position.x = -20.0f;
        if (orb.position.y < -20.0f) orb.position.y = h + 20.0f;
        if (orb.position.y > h + 20.0f) orb.position.y = -20.0f;

        orb.targetColor = nearestZoneColor(orb.position, positions);
        orb.color = lerpColor(orb.color, orb.targetColor, std::min(1.0f, dt * 2.0f));
    }
}

void Renderer::drawBackground(sf::RenderTarget& target,
                              const sf::Vector2u& windowSize,
                              const std::map<int, sf::Vector2f>& positions) const {
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    bg.setFillColor(sf::Color(8, 12, 24));
    target.draw(bg);

    const float t = gRenderClock.getElapsedTime().asSeconds();

    for (unsigned int x = 0; x < windowSize.x; x += 56) {
        const sf::Uint8 alpha = static_cast<sf::Uint8>(55 + 12 * std::sin(t * 0.7f + static_cast<float>(x) * 0.02f));
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.0f), sf::Color(25, 31, 56, alpha)),
            sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(windowSize.y)), sf::Color(25, 31, 56, alpha))
        };
        target.draw(line, 2, sf::Lines);
    }

    for (unsigned int y = 0; y < windowSize.y; y += 56) {
        const sf::Uint8 alpha = static_cast<sf::Uint8>(55 + 12 * std::sin(t * 0.7f + static_cast<float>(y) * 0.02f));
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0.0f, static_cast<float>(y)), sf::Color(25, 31, 56, alpha)),
            sf::Vertex(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(y)), sf::Color(25, 31, 56, alpha))
        };
        target.draw(line, 2, sf::Lines);
    }

    ensureAmbientOrbs(windowSize, positions);
    const float dt = std::min(0.033f, ambientClock_.restart().asSeconds());
    updateAmbientOrbs(dt, windowSize, positions);

    for (const auto& orb : ambientOrbs_) {
        const float pulse = 0.82f + 0.22f * std::sin(t * 1.4f + orb.phase);
        const float r = orb.radius * pulse;

        sf::CircleShape glow(r * 2.35f);
        glow.setOrigin(glow.getRadius(), glow.getRadius());
        glow.setPosition(orb.position);
        glow.setFillColor(sf::Color(
            orb.color.r, orb.color.g, orb.color.b,
            static_cast<sf::Uint8>(orb.color.a * 0.35f)
        ));
        target.draw(glow);

        sf::CircleShape core(r);
        core.setOrigin(core.getRadius(), core.getRadius());
        core.setPosition(orb.position);
        core.setFillColor(sf::Color(orb.color.r, orb.color.g, orb.color.b, orb.color.a));
        target.draw(core);
    }
}

void Renderer::drawEdges(sf::RenderTarget& target,
                         const std::map<int, sf::Vector2f>& positions) const {
    const float t = gRenderClock.getElapsedTime().asSeconds();

    for (auto itA = positions.begin(); itA != positions.end(); ++itA) {
        auto itB = itA;
        ++itB;

        for (; itB != positions.end(); ++itB) {
            const int idA = itA->first;
            const int idB = itB->first;
            const sf::Vector2f& a = itA->second;
            const sf::Vector2f& b = itB->second;

            sf::Vertex thick[] = {
                sf::Vertex(a, sf::Color(26, 32, 58, 190)),
                sf::Vertex(b, sf::Color(26, 32, 58, 190))
            };
            target.draw(thick, 2, sf::Lines);

            sf::Vertex core[] = {
                sf::Vertex(a, sf::Color(72, 92, 150, 150)),
                sf::Vertex(b, sf::Color(72, 92, 150, 150))
            };
            target.draw(core, 2, sf::Lines);

            const sf::Vector2f delta = b - a;
            const float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (len <= 1.0f) {
                continue;
            }

            const sf::Vector2f dir = delta / len;
            const float halfLen = len * 0.5f;

            const sf::Color colA = paletteColor(idA);
            const sf::Color colB = paletteColor(idB);

            const int wavesPerSide = 2;
            const float speed = 0.42f;
            const float spacing = 1.0f / static_cast<float>(wavesPerSide);

            for (int wave = 0; wave < wavesPerSide; ++wave) {
                const float cycle = std::fmod(t * speed + wave * spacing, 1.0f);
                const float eased = cycle * cycle * (3.0f - 2.0f * cycle);

                const sf::Vector2f pFromA = a + dir * (halfLen * eased);
                const sf::Vector2f pFromB = b - dir * (halfLen * eased);

                sf::CircleShape glowA(3.0f);
                glowA.setOrigin(glowA.getRadius(), glowA.getRadius());
                glowA.setPosition(pFromA);
                glowA.setFillColor(sf::Color(colA.r, colA.g, colA.b, 36));
                target.draw(glowA);

                sf::CircleShape dotA(1.45f);
                dotA.setOrigin(dotA.getRadius(), dotA.getRadius());
                dotA.setPosition(pFromA);
                dotA.setFillColor(sf::Color(colA.r, colA.g, colA.b, 175));
                target.draw(dotA);

                sf::CircleShape glowB(3.0f);
                glowB.setOrigin(glowB.getRadius(), glowB.getRadius());
                glowB.setPosition(pFromB);
                glowB.setFillColor(sf::Color(colB.r, colB.g, colB.b, 36));
                target.draw(glowB);

                sf::CircleShape dotB(1.45f);
                dotB.setOrigin(dotB.getRadius(), dotB.getRadius());
                dotB.setPosition(pFromB);
                dotB.setFillColor(sf::Color(colB.r, colB.g, colB.b, 175));
                target.draw(dotB);
            }
        }
    }
}

void Renderer::drawInfoButton(sf::RenderTarget& target, const sf::Vector2u& windowSize, bool hovered) const {
    sf::RectangleShape button({34.0f, 34.0f});
    button.setPosition(infoButtonRect(windowSize).left, infoButtonRect(windowSize).top);
    button.setFillColor(hovered ? sf::Color(55, 70, 110) : sf::Color(32, 40, 66));
    button.setOutlineThickness(2.0f);
    button.setOutlineColor(sf::Color(100, 120, 170));
    target.draw(button);

    sf::Text text;
    text.setFont(font_);
    text.setString("i");
    text.setCharacterSize(20);
    text.setStyle(sf::Text::Bold);
    text.setFillColor(sf::Color::White);
    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(infoButtonRect(windowSize).left + 17.0f, infoButtonRect(windowSize).top + 17.0f);
    target.draw(text);
}

void Renderer::drawTooltip(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const {
    sf::RectangleShape panel({420.0f, 140.0f});
    panel.setPosition(static_cast<float>(windowSize.x) - 440.0f, 58.0f);
    panel.setFillColor(sf::Color(12, 18, 32, 235));
    panel.setOutlineThickness(2.0f);
    panel.setOutlineColor(sf::Color(80, 100, 150));
    target.draw(panel);

    sf::Text text;
    text.setFont(font_);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);

    if (towerCount == 2) {
        text.setString("T1: 1 attack, 2 heal\nT2: Q attack, W heal\nL = autoplay on/off\nF5 reset, ESC exit");
    } else if (towerCount == 3) {
        text.setString("T1: 1 2 attack, 3 heal\nT2: Q W attack, E heal\nT3: A S attack, D heal\nL = autoplay on/off");
    } else {
        text.setString("T1: 1 2 3 attack, 4 heal\nT2: Q W E attack, R heal\nT3: A S D attack, F heal\nT4: Z X C attack, V heal\nL = autoplay on/off");
    }

    text.setPosition(static_cast<float>(windowSize.x) - 424.0f, 74.0f);
    target.draw(text);
}

void Renderer::drawTeamLabel(sf::RenderTarget& target, const sf::Vector2u& windowSize, std::size_t towerCount) const {
    (void)windowSize;

    sf::Text label;
    label.setFont(font_);
    label.setCharacterSize(28);
    label.setStyle(sf::Text::Bold);
    label.setString("TIKTOK TOWER WAR");
    label.setFillColor(sf::Color::White);
    label.setPosition(22.0f, 18.0f);
    target.draw(label);

    sf::Text subtitle;
    subtitle.setFont(font_);
    subtitle.setCharacterSize(16);
    subtitle.setFillColor(sf::Color(180, 190, 215));
    subtitle.setString(std::to_string(towerCount) + " towers • autoplay on L • dynamic colored background");
    subtitle.setPosition(24.0f, 54.0f);
    // target.draw(subtitle);
}

void Renderer::render(sf::RenderWindow& window, const GameState& state, Team viewerTeam) {
    (void)viewerTeam;

    const auto size = window.getSize();
    const auto positions = towerPositions(size, state.towers().size());
    const auto mousePos = sf::Mouse::getPosition(window);
    const bool hovered = infoButtonRect(size).contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

    window.clear(sf::Color(7, 10, 20));
    drawBackground(window, size, positions);
    drawPromoText(window, size, font_);
    drawEdges(window, positions);
    unitRenderer_.render(window, state, positions, config_.towerRadius, config_.unitRadius);
    towerRenderer_.render(window, state, positions, config_.towerRadius, font_);
    effectsRenderer_.render(window, state, positions, config_.towerRadius, font_);
    drawTeamLabel(window, size, state.towers().size());
    drawInfoButton(window, size, hovered);
    if (hovered) {
        drawTooltip(window, size, state.towers().size());
    }

    window.display();
}



} // namespace tw