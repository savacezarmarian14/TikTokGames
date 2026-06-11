#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "audio/AudioManager.hpp"
#include "core/Config.hpp"
#include "core/Types.hpp"
#include "input/InputManager.hpp"
#include "model/GameState.hpp"
#include "render/Renderer.hpp"
#include "systems/CollisionSystem.hpp"
#include "systems/CombatSystem.hpp"
#include "systems/MovementSystem.hpp"
#include "systems/RoundSystem.hpp"
#include "systems/SpawnSystem.hpp"

namespace tw {

/**
 * @brief Owns the application loop, systems, state, and SFML windows.
 */
class Game {
public:
    /** @brief Creates a game with at least two towers. */
    explicit Game(std::size_t towerCount);
    /** @brief Creates a game with a custom runtime configuration. */
    Game(std::size_t towerCount, Config config);
    /** @brief Runs the main process-update-render loop until all windows close. */
    void run();

private:
    struct WindowContext {
        Team team = Team::None;
        std::string title;
        sf::RenderWindow window;
        Renderer renderer;

        WindowContext(const Config& config, Team teamValue, const std::string& titleValue)
            : team(teamValue),
              title(titleValue),
              window(),
              renderer(config) {}
    };

private:
    /** @brief Creates all render windows used by the game. */
    void createWindows();
    /** @brief Returns the SFML window style for the current presentation mode. */
    unsigned int currentWindowStyle() const;
    /** @brief Recreates a window when switching between framed and borderless mode. */
    void recreateWindow(WindowContext& ctx, const sf::Vector2u& size, const sf::Vector2i& position);
    /** @brief Rebuilds towers, clears transient state, and resets autoplay. */
    void initializeState();
    /** @brief Starts a fresh round. */
    void resetState();

    /** @brief Dispatches pending SFML events for every window. */
    void processEvents();
    /** @brief Handles close, hotkey, and gameplay input for one window. */
    void processWindowEvents(WindowContext& ctx);
    /** @brief Advances simulation by one frame. */
    void update(float dt);
    /** @brief Draws all open windows. */
    void render();

    /** @brief Returns true when no window remains open. */
    bool allWindowsClosed() const;
    /** @brief Closes every open window. */
    void closeAllWindows();

    /** @brief Chooses a window size from the current desktop resolution. */
    sf::Vector2u computeWindowSize() const;

    /** @brief Enables or disables autonomous command generation. */
    void toggleAutoplay();
    /** @brief Toggles borderless clean presentation mode. */
    void togglePresentationMode();
    /** @brief Applies generated autoplay commands for one frame. */
    void updateAutoplay(float dt);
    /** @brief Builds randomized commands that keep the game moving. */
    std::vector<GameCommand> buildAutoplayCommands(float dt);

private:
    Config config_;
    std::size_t towerCount_ = 3;
    std::vector<std::unique_ptr<WindowContext>> windows_;

    GameState state_;
    InputManager inputManager_;

    SpawnSystem spawnSystem_;
    MovementSystem movementSystem_;
    CollisionSystem collisionSystem_;
    CombatSystem combatSystem_;
    RoundSystem roundSystem_;
    AudioManager audioManager_;

    bool autoplayEnabled_ = false;
    bool presentationMode_ = false;
    bool autoplayBurstMode_ = false;
    float autoplayModeTimer_ = 0.0f;
    float autoplayNextShotTimer_ = 0.0f;

    std::mt19937 rng_;
};

} // namespace tw
