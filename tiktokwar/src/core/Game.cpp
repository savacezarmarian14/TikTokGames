#include "core/Game.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

namespace tw {

Game::Game(std::size_t towerCount)
    : Game(towerCount, Config()) {}

Game::Game(std::size_t towerCount, Config config)
    : config_(std::move(config)),
      towerCount_(std::max<std::size_t>(2, towerCount)),
      windows_(),
      state_(),
      inputManager_(config_.networkInput),
      spawnSystem_(config_),
      movementSystem_(),
      collisionSystem_(),
      combatSystem_(config_),
      roundSystem_(),
      rng_(static_cast<unsigned int>(
          std::chrono::high_resolution_clock::now().time_since_epoch().count())) {
    initializeState();
    createWindows();
}

void Game::initializeState() {
    state_ = GameState();

    for (std::size_t i = 0; i < towerCount_; ++i) {
        const auto team = static_cast<Team>(static_cast<int>(i));
        state_.towers().emplace_back(
            static_cast<int>(i),
            team,
            "T" + std::to_string(i + 1),
            config_.towerMaxHp
        );
    }

    state_.setRoundActive(true);
    state_.setWinnerId(-1);
    state_.clearUnits();
    state_.clearEvents();

    autoplayEnabled_ = false;
    autoplayBurstMode_ = false;
    autoplayModeTimer_ = 0.0f;
    autoplayNextShotTimer_ = 0.0f;
}

void Game::resetState() {
    initializeState();
}

void Game::run() {
    sf::Clock frameClock;

    while (!allWindowsClosed()) {
        const float dt = frameClock.restart().asSeconds();

        processEvents();
        update(dt);
        render();
    }
}

void Game::createWindows() {
    windows_.clear();
    windows_.reserve(1);

    const sf::Vector2u windowSize = computeWindowSize();

    auto ctx = std::make_unique<WindowContext>(config_, Team::None, "TikTok Tower War");
    recreateWindow(*ctx, windowSize, {120, 80});

    windows_.push_back(std::move(ctx));
}

unsigned int Game::currentWindowStyle() const {
    return presentationMode_
        ? sf::Style::None
        : sf::Style::Titlebar | sf::Style::Resize | sf::Style::Close;
}

void Game::recreateWindow(WindowContext& ctx, const sf::Vector2u& size, const sf::Vector2i& position) {
    ctx.window.create(
        sf::VideoMode(size.x, size.y),
        ctx.title,
        currentWindowStyle()
    );
    ctx.window.setFramerateLimit(config_.fpsLimit);
    ctx.window.setPosition(position);
    ctx.window.clear(sf::Color(7, 10, 20));
    ctx.window.display();
}

sf::Vector2u Game::computeWindowSize() const {
    const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    return {
        std::max(static_cast<unsigned int>(std::max(1, config_.windowWidth)), desktop.width * 3 / 4),
        std::max(static_cast<unsigned int>(std::max(1, config_.windowHeight)), desktop.height * 3 / 4)
    };
}

bool Game::allWindowsClosed() const {
    for (const auto& ctx : windows_) {
        if (ctx && ctx->window.isOpen()) {
            return false;
        }
    }
    return true;
}

void Game::closeAllWindows() {
    for (auto& ctx : windows_) {
        if (ctx && ctx->window.isOpen()) {
            ctx->window.close();
        }
    }
}

void Game::processEvents() {
    for (auto& ctx : windows_) {
        if (ctx) {
            processWindowEvents(*ctx);
        }
    }
}

void Game::processWindowEvents(WindowContext& ctx) {
    if (!ctx.window.isOpen()) {
        return;
    }

    sf::Event event;
    while (ctx.window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            ctx.window.close();
            continue;
        }

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                closeAllWindows();
                return;
            }

            if (event.key.code == sf::Keyboard::F5) {
                resetState();
                continue;
            }

            if (event.key.code == sf::Keyboard::L) {
                toggleAutoplay();
                continue;
            }

            if (event.key.code == sf::Keyboard::Space) {
                togglePresentationMode();
                return;
            }
        }

        inputManager_.handleEvent(event, state_.towers().size());
    }
}

void Game::toggleAutoplay() {
    autoplayEnabled_ = !autoplayEnabled_;

    if (autoplayEnabled_) {
        autoplayBurstMode_ = false;
        autoplayModeTimer_ = 0.15f;
        autoplayNextShotTimer_ = 0.04f;
    }
}

void Game::togglePresentationMode() {
    presentationMode_ = !presentationMode_;

    for (auto& ctx : windows_) {
        if (!ctx || !ctx->window.isOpen()) {
            continue;
        }

        const sf::Vector2u size = ctx->window.getSize();
        const sf::Vector2i position = ctx->window.getPosition();
        recreateWindow(*ctx, size, position);
    }
}

std::vector<GameCommand> Game::buildAutoplayCommands(float dt) {
    std::vector<GameCommand> commands;

    if (!autoplayEnabled_ || !state_.isRoundActive() || state_.towers().size() < 2) {
        return commands;
    }

    autoplayModeTimer_ -= dt;
    autoplayNextShotTimer_ -= dt;

    if (autoplayModeTimer_ <= 0.0f) {
        std::uniform_real_distribution<float> modeRoll(0.0f, 1.0f);

        autoplayBurstMode_ = modeRoll(rng_) < 0.58f;

        if (autoplayBurstMode_) {
            std::uniform_real_distribution<float> burstDuration(0.55f, 1.3f);
            autoplayModeTimer_ = burstDuration(rng_);
        } else {
            std::uniform_real_distribution<float> idleDuration(0.18f, 0.55f);
            autoplayModeTimer_ = idleDuration(rng_);
        }
    }

    if (autoplayNextShotTimer_ > 0.0f) {
        return commands;
    }

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_int_distribution<int> amountDistBurst(1, 3);
    std::uniform_int_distribution<int> amountDistShort(1, 2);

    if (autoplayBurstMode_) {
        std::uniform_real_distribution<float> nextBurstDelay(0.025f, 0.075f);
        autoplayNextShotTimer_ = nextBurstDelay(rng_);
    } else {
        std::uniform_real_distribution<float> nextShortDelay(0.08f, 0.20f);
        autoplayNextShotTimer_ = nextShortDelay(rng_);
    }

    std::vector<int> towerOrder;
    towerOrder.reserve(state_.towers().size());
    for (const auto& tower : state_.towers()) {
        if (tower.isAlive()) {
            towerOrder.push_back(tower.id());
        }
    }

    std::shuffle(towerOrder.begin(), towerOrder.end(), rng_);

    for (int towerId : towerOrder) {
        const auto* towerPtr = state_.findTowerById(towerId);
        if (!towerPtr) {
            continue;
        }

        const auto& tower = *towerPtr;
        if (!tower.isAlive()) {
            continue;
        }

        float skipChance = autoplayBurstMode_ ? 0.18f : 0.28f;
        if (chance(rng_) < skipChance) {
            continue;
        }

        std::vector<int> aliveTargets;
        for (const auto& other : state_.towers()) {
            if (other.id() != tower.id() && other.isAlive()) {
                aliveTargets.push_back(other.id());
            }
        }

        if (aliveTargets.empty()) {
            continue;
        }

        int activeFromThisTower = 0;
        for (const auto& unitPtr : state_.units()) {
            if (unitPtr && unitPtr->sourceTowerId() == tower.id()) {
                ++activeFromThisTower;
            }
        }

        const int softCap = autoplayBurstMode_ ? 18 : 12;
        if (activeFromThisTower >= softCap) {
            continue;
        }

        const float healChance = autoplayBurstMode_ ? 0.10f : 0.20f;
        const float attackChance = autoplayBurstMode_ ? 0.72f : 0.55f;

        const float roll = chance(rng_);

        if (roll < healChance) {
            commands.push_back({
                CommandType::SpawnHeal,
                tower.team(),
                tower.id(),
                tower.id(),
                1
            });
            continue;
        }

        if (roll > healChance + attackChance) {
            continue;
        }

        std::uniform_int_distribution<int> targetPicker(0, static_cast<int>(aliveTargets.size()) - 1);
        const int targetId = aliveTargets[targetPicker(rng_)];
        const int amount = autoplayBurstMode_ ? amountDistBurst(rng_) : amountDistShort(rng_);

        commands.push_back({
            CommandType::SpawnAttack,
            tower.team(),
            tower.id(),
            targetId,
            amount
        });
    }

    return commands;
}

void Game::updateAutoplay(float dt) {
    auto autoCommands = buildAutoplayCommands(dt);
    if (!autoCommands.empty()) {
        spawnSystem_.apply(state_, autoCommands);
    }
}

void Game::update(float dt) {
    state_.clearEvents();

    inputManager_.poll(state_.towers().size());
    const auto commands = inputManager_.consumeCommands();
    spawnSystem_.apply(state_, commands);

    updateAutoplay(dt);

    movementSystem_.update(state_, dt);

    if (!windows_.empty() && windows_[0]) {
        const auto positions = windows_[0]->renderer.towerPositions(
            windows_[0]->renderer.virtualCanvasSize(),
            state_.towers().size()
        );
        const float radius = config_.towerRadius;

        collisionSystem_.update(state_, positions, radius, config_.unitRadius);
        combatSystem_.update(state_, positions, radius);
    }

    roundSystem_.update(state_);
    audioManager_.playEvents(state_);
}

void Game::render() {
    for (auto& ctx : windows_) {
        if (!ctx || !ctx->window.isOpen()) {
            continue;
        }

        ctx->renderer.render(ctx->window, state_, ctx->team, presentationMode_);
    }
}

} // namespace tw
