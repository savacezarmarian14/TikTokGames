#include "render/Renderer.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tw {

namespace {

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

std::vector<std::string> parseGiftTeamNames(const std::string& xml) {
    std::vector<std::string> teamNames;
    std::size_t cursor = 0;

    while (true) {
        const std::size_t tagStart = xml.find('<', cursor);
        if (tagStart == std::string::npos) {
            break;
        }

        const std::size_t nameStart = tagStart + 1;
        if (nameStart >= xml.size()) {
            break;
        }

        const char first = xml[nameStart];
        if (first == '/' || first == '?' || first == '!') {
            cursor = nameStart + 1;
            continue;
        }

        std::size_t nameEnd = nameStart;
        while (nameEnd < xml.size()) {
            const char c = xml[nameEnd];
            if (c == '>' || c == '/' || std::isspace(static_cast<unsigned char>(c))) {
                break;
            }
            ++nameEnd;
        }

        const std::string name = xml.substr(nameStart, nameEnd - nameStart);
        if (name != "Gifts" && name != "gift" &&
            std::find(teamNames.begin(), teamNames.end(), name) == teamNames.end()) {
            teamNames.push_back(name);
        }

        cursor = nameEnd;
    }

    return teamNames;
}

int giftTeamId(const std::string& name, const std::vector<std::string>& teamNames) {
    const auto it = std::find(teamNames.begin(), teamNames.end(), name);
    if (it == teamNames.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(teamNames.begin(), it));
}

std::filesystem::path resolveProjectPath(const std::filesystem::path& relativePath) {
    std::filesystem::path current = std::filesystem::current_path();

    for (int depth = 0; depth < 6; ++depth) {
        const std::filesystem::path candidate = current / relativePath;
        if (std::filesystem::is_regular_file(candidate) || std::filesystem::is_directory(candidate)) {
            return candidate;
        }

        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    return relativePath;
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string attributeValue(const std::string& tag, const std::string& attributeName) {
    const std::string needle = attributeName + "=\"";
    const std::size_t start = tag.find(needle);
    if (start == std::string::npos) {
        return {};
    }

    const std::size_t valueStart = start + needle.size();
    const std::size_t valueEnd = tag.find('"', valueStart);
    if (valueEnd == std::string::npos) {
        return {};
    }

    return tag.substr(valueStart, valueEnd - valueStart);
}

} // namespace

Renderer::Renderer(const Config& config)
    : config_(config),
      viewportScaler_({
          static_cast<float>(std::max(1, config.windowWidth)),
          static_cast<float>(std::max(1, config.windowHeight))
      }) {}

sf::Vector2u Renderer::virtualCanvasSize() const {
    return viewportScaler_.virtualPixelSize();
}

void Renderer::ensureFontLoaded() const {
    if (fontLoadAttempted_) {
        return;
    }

    fontLoadAttempted_ = true;
    const std::vector<std::filesystem::path> fontCandidates = {
        resolveProjectPath("assets/fonts/DejaVuSans-Bold.ttf"),
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf"
    };

    for (const auto& fontPath : fontCandidates) {
        if (font_.loadFromFile(fontPath.string())) {
            fontLoaded_ = true;
            break;
        }
    }

    if (!fontLoaded_) {
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

    const sf::Vector2f center(w * 0.5f, h * 0.5f + 40.0f);
    const float usableRadius = std::min(w, h) * 0.32f;
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

    constexpr int orbCount = 36;
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
                              const std::map<int, sf::Vector2f>& positions,
                              bool showGrid) const {
    sf::RectangleShape bg(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    bg.setFillColor(sf::Color(8, 12, 24));
    target.draw(bg);

    const float t = gRenderClock.getElapsedTime().asSeconds();

    if (showGrid) {
        for (unsigned int x = 0; x < windowSize.x; x += 72) {
            const sf::Uint8 alpha = static_cast<sf::Uint8>(42 + 8 * std::sin(t * 0.7f + static_cast<float>(x) * 0.02f));
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.0f), sf::Color(25, 31, 56, alpha)),
                sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(windowSize.y)), sf::Color(25, 31, 56, alpha))
            };
            target.draw(line, 2, sf::Lines);
        }

        for (unsigned int y = 0; y < windowSize.y; y += 72) {
            const sf::Uint8 alpha = static_cast<sf::Uint8>(42 + 8 * std::sin(t * 0.7f + static_cast<float>(y) * 0.02f));
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(0.0f, static_cast<float>(y)), sf::Color(25, 31, 56, alpha)),
                sf::Vertex(sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(y)), sf::Color(25, 31, 56, alpha))
            };
            target.draw(line, 2, sf::Lines);
        }
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

            const sf::Color colA = paletteColor(idA);
            const sf::Color colB = paletteColor(idB);

            sf::Vertex thick[] = {
                sf::Vertex(a, sf::Color(colA.r / 2, colA.g / 2, colA.b / 2, 88)),
                sf::Vertex(b, sf::Color(colB.r / 2, colB.g / 2, colB.b / 2, 88))
            };
            target.draw(thick, 2, sf::Lines);

            sf::Vertex core[] = {
                sf::Vertex(a, sf::Color(colA.r, colA.g, colA.b, 170)),
                sf::Vertex(b, sf::Color(colB.r, colB.g, colB.b, 170))
            };
            target.draw(core, 2, sf::Lines);

            const sf::Vector2f delta = b - a;
            const float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (len <= 1.0f) {
                continue;
            }

            const sf::Vector2f dir = delta / len;
            const float halfLen = len * 0.5f;

            const int wavesPerSide = 1;
            const float speed = 0.55f;
            const float spacing = 1.0f / static_cast<float>(wavesPerSide);

            for (int wave = 0; wave < wavesPerSide; ++wave) {
                const float cycle = std::fmod(t * speed + wave * spacing, 1.0f);
                const float eased = cycle * cycle * (3.0f - 2.0f * cycle);

                const sf::Vector2f pFromA = a + dir * (halfLen * eased);
                const sf::Vector2f pFromB = b - dir * (halfLen * eased);

                sf::CircleShape glowA(3.8f);
                glowA.setOrigin(glowA.getRadius(), glowA.getRadius());
                glowA.setPosition(pFromA);
                glowA.setFillColor(sf::Color(colA.r, colA.g, colA.b, 58));
                target.draw(glowA);

                sf::CircleShape dotA(1.8f);
                dotA.setOrigin(dotA.getRadius(), dotA.getRadius());
                dotA.setPosition(pFromA);
                dotA.setFillColor(sf::Color(255, 255, 255, 210));
                target.draw(dotA);

                sf::CircleShape glowB(3.8f);
                glowB.setOrigin(glowB.getRadius(), glowB.getRadius());
                glowB.setPosition(pFromB);
                glowB.setFillColor(sf::Color(colB.r, colB.g, colB.b, 58));
                target.draw(glowB);

                sf::CircleShape dotB(1.8f);
                dotB.setOrigin(dotB.getRadius(), dotB.getRadius());
                dotB.setPosition(pFromB);
                dotB.setFillColor(sf::Color(255, 255, 255, 210));
                target.draw(dotB);
            }
        }
    }
}

void Renderer::ensureGiftCatalogLoaded() const {
    if (giftCatalogLoadAttempted_) {
        return;
    }

    giftCatalogLoadAttempted_ = true;
    attackGiftVisuals_.clear();
    healGiftVisuals_.clear();

    const std::filesystem::path giftsPath = resolveProjectPath("gifts.xml");
    const std::string xml = readTextFile(giftsPath);
    if (xml.empty()) {
        std::cerr << "Warning: failed to load gifts.xml. Attack gift icons will not render.\n";
        return;
    }

    const std::vector<std::string> teamNames = parseGiftTeamNames(xml);
    for (const auto& sourceName : teamNames) {
        const int sourceId = giftTeamId(sourceName, teamNames);
        const std::string openTag = "<" + sourceName + ">";
        const std::string closeTag = "</" + sourceName + ">";
        const std::size_t blockStart = xml.find(openTag);
        const std::size_t blockEnd = xml.find(closeTag);
        if (sourceId < 0 || blockStart == std::string::npos || blockEnd == std::string::npos || blockEnd <= blockStart) {
            continue;
        }

        const std::string block = xml.substr(blockStart + openTag.size(), blockEnd - blockStart - openTag.size());
        std::size_t cursor = 0;
        while (true) {
            const std::size_t tagStart = block.find("<gift", cursor);
            if (tagStart == std::string::npos) {
                break;
            }

            const std::size_t tagEnd = block.find("/>", tagStart);
            if (tagEnd == std::string::npos) {
                break;
            }

            const std::string tag = block.substr(tagStart, tagEnd - tagStart + 2);
            cursor = tagEnd + 2;

            const std::string action = attributeValue(tag, "action");
            const int targetId = giftTeamId(attributeValue(tag, "target"), teamNames);
            const std::string path = attributeValue(tag, "path");
            if (targetId < 0 || path.empty()) {
                continue;
            }

            if (action == "spawn") {
                attackGiftVisuals_.push_back({
                    sourceId,
                    targetId,
                    attributeValue(tag, "name"),
                    path
                });
            } else if (action == "heal") {
                healGiftVisuals_.push_back({
                    targetId,
                    attributeValue(tag, "name"),
                    path
                });
            }
        }
    }
}

const sf::Texture* Renderer::textureForGift(const std::string& path) const {
    const auto loadedIt = giftTextures_.find(path);
    if (loadedIt != giftTextures_.end()) {
        return &loadedIt->second;
    }

    if (failedGiftTextures_.find(path) != failedGiftTextures_.end()) {
        return nullptr;
    }

    sf::Texture texture;
    const std::filesystem::path resolvedPath = resolveProjectPath(path);
    if (!texture.loadFromFile(resolvedPath.string())) {
        failedGiftTextures_.insert(path);
        return nullptr;
    }

    texture.setSmooth(true);
    auto [insertedIt, inserted] = giftTextures_.emplace(path, std::move(texture));
    (void)inserted;
    return &insertedIt->second;
}

void Renderer::drawGiftIcon(sf::RenderTarget& target,
                            const sf::Texture& texture,
                            const sf::Vector2f& center,
                            const sf::Color& accentColor,
                            float plateRadius,
                            float iconSize) const {
    sf::CircleShape glow(plateRadius + 5.0f);
    glow.setOrigin(glow.getRadius(), glow.getRadius());
    glow.setPosition(center);
    glow.setFillColor(sf::Color(accentColor.r, accentColor.g, accentColor.b, 44));
    target.draw(glow);

    sf::CircleShape plate(plateRadius);
    plate.setOrigin(plate.getRadius(), plate.getRadius());
    plate.setPosition(center);
    plate.setFillColor(sf::Color(9, 14, 28, 214));
    plate.setOutlineThickness(1.4f);
    plate.setOutlineColor(sf::Color(accentColor.r, accentColor.g, accentColor.b, 172));
    target.draw(plate);

    sf::Sprite sprite(texture);
    const sf::Vector2u size = texture.getSize();
    if (size.x == 0 || size.y == 0) {
        return;
    }

    const float scale = iconSize / static_cast<float>(std::max(size.x, size.y));
    sprite.setOrigin(static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.5f);
    sprite.setScale(scale, scale);
    sprite.setPosition(center);
    target.draw(sprite);
}

void Renderer::drawAttackGiftIcons(sf::RenderTarget& target,
                                   const GameState& state,
                                   const std::map<int, sf::Vector2f>& positions) const {
    ensureGiftCatalogLoaded();
    if (attackGiftVisuals_.empty()) {
        return;
    }

    auto towerIsAlive = [&state](int towerId) {
        const auto* tower = state.findTowerById(towerId);
        return tower && tower->isAlive();
    };

    sf::Vector2f arenaCenter;
    for (const auto& [id, pos] : positions) {
        (void)id;
        arenaCenter += pos;
    }
    arenaCenter /= static_cast<float>(positions.size());

    std::unordered_map<std::string, int> laneGiftSlots;

    for (const auto& gift : attackGiftVisuals_) {
        if (!towerIsAlive(gift.sourceTowerId) || !towerIsAlive(gift.targetTowerId)) {
            continue;
        }

        const auto sourceIt = positions.find(gift.sourceTowerId);
        const auto targetIt = positions.find(gift.targetTowerId);
        if (sourceIt == positions.end() || targetIt == positions.end()) {
            continue;
        }

        const sf::Texture* texture = textureForGift(gift.texturePath);
        if (!texture) {
            continue;
        }

        const sf::Vector2f source = sourceIt->second;
        const sf::Vector2f destination = targetIt->second;
        const sf::Vector2f delta = destination - source;
        const float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (len <= 1.0f) {
            continue;
        }

        const sf::Vector2f dir = delta / len;
        const sf::Vector2f normal(-dir.y, dir.x);
        sf::Vector2f side = normal;
        if (std::abs(side.y) > 0.08f) {
            if (side.y < 0.0f) {
                side *= -1.0f;
            }
        } else {
            constexpr float giftSideOffset = 30.0f;
            const sf::Vector2f lanePoint = source + dir * (config_.towerRadius + 70.0f);
            const float outwardDist = lengthSquared((lanePoint + side * giftSideOffset) - arenaCenter);
            const float inwardDist = lengthSquared((lanePoint - side * giftSideOffset) - arenaCenter);
            if (inwardDist > outwardDist) {
                side *= -1.0f;
            }
        }

        const std::string laneKey = std::to_string(gift.sourceTowerId) + ":" +
            std::to_string(gift.targetTowerId);
        const int laneSlot = laneGiftSlots[laneKey]++;
        constexpr float giftSideOffset = 30.0f;
        constexpr float giftSlotSpacing = 42.0f;
        const float laneDistance = config_.towerRadius + 70.0f +
            static_cast<float>(laneSlot) * giftSlotSpacing;
        const sf::Vector2f iconCenter = source + dir * laneDistance + side * giftSideOffset;
        const sf::Color sourceColor = paletteColor(gift.sourceTowerId);
        drawGiftIcon(target, *texture, iconCenter, sourceColor, 18.0f, 27.0f);
    }
}

void Renderer::drawHealGiftIcons(sf::RenderTarget& target,
                                 const GameState& state,
                                 const std::map<int, sf::Vector2f>& positions) const {
    ensureGiftCatalogLoaded();
    if (healGiftVisuals_.empty() || positions.empty()) {
        return;
    }

    sf::Vector2f arenaCenter;
    for (const auto& [id, pos] : positions) {
        (void)id;
        arenaCenter += pos;
    }
    arenaCenter /= static_cast<float>(positions.size());

    for (const auto& gift : healGiftVisuals_) {
        const auto* tower = state.findTowerById(gift.towerId);
        if (!tower || !tower->isAlive()) {
            continue;
        }

        const auto towerIt = positions.find(gift.towerId);
        if (towerIt == positions.end()) {
            continue;
        }

        const sf::Texture* texture = textureForGift(gift.texturePath);
        if (!texture) {
            continue;
        }

        const sf::Vector2f towerPos = towerIt->second;
        sf::Vector2f radial = towerPos - arenaCenter;
        const float radialLen = std::sqrt(radial.x * radial.x + radial.y * radial.y);
        if (radialLen <= 1.0f) {
            radial = {0.0f, -1.0f};
        } else {
            radial /= radialLen;
        }

        const sf::Vector2f iconCenter = towerPos + radial * (config_.towerRadius + 50.0f);
        const sf::Color healColor(95, 255, 150);
        const sf::Color towerColor = paletteColor(gift.towerId);
        const sf::Color accent(
            static_cast<sf::Uint8>((static_cast<int>(healColor.r) + static_cast<int>(towerColor.r)) / 2),
            static_cast<sf::Uint8>((static_cast<int>(healColor.g) + static_cast<int>(towerColor.g)) / 2),
            static_cast<sf::Uint8>((static_cast<int>(healColor.b) + static_cast<int>(towerColor.b)) / 2)
        );
        drawGiftIcon(target, *texture, iconCenter, accent, 20.0f, 30.0f);
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

void Renderer::render(sf::RenderWindow& window, const GameState& state, Team viewerTeam, bool cleanView) {
    (void)viewerTeam;
    ensureFontLoaded();

    const sf::View previousView = window.getView();
    window.setView(viewportScaler_.viewFor(window.getSize()));

    const auto size = virtualCanvasSize();
    const auto positions = towerPositions(size, state.towers().size());
    const sf::Vector2f mousePos = viewportScaler_.mapPixelToVirtual(window, sf::Mouse::getPosition(window));
    const bool hovered = infoButtonRect(size).contains(mousePos.x, mousePos.y);

    window.clear(sf::Color(7, 10, 20));
    drawBackground(window, size, positions, !cleanView);
    drawEdges(window, positions);
    drawAttackGiftIcons(window, state, positions);
    drawHealGiftIcons(window, state, positions);
    unitRenderer_.render(window, state, positions, config_.towerRadius, config_.unitRadius);
    towerRenderer_.render(window, state, positions, config_.towerRadius, font_);
    effectsRenderer_.render(window, state, positions, config_.towerRadius, font_);
    if (!cleanView) {
        drawTeamLabel(window, size, state.towers().size());
    }
    drawInfoButton(window, size, hovered);
    if (hovered) {
        drawTooltip(window, size, state.towers().size());
    }

    window.setView(previousView);
    window.display();
}



} // namespace tw
