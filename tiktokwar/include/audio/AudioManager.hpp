#pragma once

#include <SFML/Audio.hpp>
#include <array>

#include "model/GameState.hpp"

namespace tw {

/**
 * @brief Plays short sound effects for frame events.
 */
class AudioManager {
public:
    /** @brief Loads sound buffers and prepares reusable playback channels. */
    AudioManager();

    /** @brief Plays audio associated with events emitted this frame. */
    void playEvents(const GameState& state);

private:
    /** @brief Returns an idle sound channel or nullptr when all are busy. */
    sf::Sound* acquireFreeChannel();
    /** @brief Plays a sound buffer if its cooldown has elapsed. */
    void playSound(sf::SoundBuffer& buffer, sf::Int32& lastPlayMs, sf::Int32 cooldownMs, float volume);

private:
    sf::SoundBuffer spawnBuffer_;
    sf::SoundBuffer hitBuffer_;
    sf::SoundBuffer healBuffer_;
    sf::SoundBuffer destroyBuffer_;

    static constexpr std::size_t kChannelCount = 24;
    std::array<sf::Sound, kChannelCount> channels_{};

    sf::Clock audioClock_;

    sf::Int32 lastSpawnMs_ = -100000;
    sf::Int32 lastHitMs_ = -100000;
    sf::Int32 lastHealMs_ = -100000;
    sf::Int32 lastDestroyMs_ = -100000;

    bool spawnLoaded_ = false;
    bool hitLoaded_ = false;
    bool healLoaded_ = false;
    bool destroyLoaded_ = false;
};

} // namespace tw
