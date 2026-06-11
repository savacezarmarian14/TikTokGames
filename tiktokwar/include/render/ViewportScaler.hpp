#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>

namespace tw {

/**
 * @brief Builds a letterboxed SFML view for a fixed virtual game canvas.
 */
class ViewportScaler {
public:
    explicit ViewportScaler(sf::Vector2f virtualSize)
        : virtualSize_(std::max(1.0f, virtualSize.x), std::max(1.0f, virtualSize.y)) {}

    sf::Vector2f virtualSize() const {
        return virtualSize_;
    }

    sf::Vector2u virtualPixelSize() const {
        return {
            static_cast<unsigned int>(std::round(virtualSize_.x)),
            static_cast<unsigned int>(std::round(virtualSize_.y))
        };
    }

    sf::View viewFor(const sf::Vector2u& windowSize) const {
        sf::View view(sf::FloatRect(0.0f, 0.0f, virtualSize_.x, virtualSize_.y));
        view.setViewport(viewportFor(windowSize));
        return view;
    }

    sf::Vector2f mapPixelToVirtual(const sf::RenderWindow& window, const sf::Vector2i& pixel) const {
        return window.mapPixelToCoords(pixel, viewFor(window.getSize()));
    }

private:
    sf::FloatRect viewportFor(const sf::Vector2u& windowSize) const {
        if (windowSize.x == 0 || windowSize.y == 0) {
            return {0.0f, 0.0f, 1.0f, 1.0f};
        }

        const float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        const float virtualRatio = virtualSize_.x / virtualSize_.y;

        if (windowRatio > virtualRatio) {
            const float width = virtualRatio / windowRatio;
            return {(1.0f - width) * 0.5f, 0.0f, width, 1.0f};
        }

        const float height = windowRatio / virtualRatio;
        return {0.0f, (1.0f - height) * 0.5f, 1.0f, height};
    }

private:
    sf::Vector2f virtualSize_;
};

} // namespace tw
