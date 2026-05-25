#pragma once

#include <SFML/Graphics.hpp>
#include "core/Types.hpp"
#include "render/Renderer.hpp"

namespace tw {

struct WindowContext {
    sf::RenderWindow window;
    Team team = Team::None;
    Renderer renderer;

    WindowContext(const Config& config, Team t)
        : window(),
          team(t),
          renderer(config) {}
};

} // namespace tw