#include "audio/AudioManager.hpp"

namespace tw {

AudioManager::AudioManager() {
    spawnLoaded_ = spawnBuffer_.loadFromFile("../assets/sounds/spawn.wav");
    hitLoaded_ = hitBuffer_.loadFromFile("../assets/sounds/hit.wav");
    healLoaded_ = healBuffer_.loadFromFile("../assets/sounds/heal.wav");
    destroyLoaded_ = destroyBuffer_.loadFromFile("../assets/sounds/destroy.wav");

    for (auto& channel : channels_) {
        channel.setVolume(100.0f);
    }
}

sf::Sound* AudioManager::acquireFreeChannel() {
    for (auto& channel : channels_) {
        if (channel.getStatus() != sf::Sound::Playing) {
            return &channel;
        }
    }
    return nullptr;
}

void AudioManager::playSound(sf::SoundBuffer& buffer,
                             sf::Int32& lastPlayMs,
                             sf::Int32 cooldownMs,
                             float volume) {
    const sf::Int32 now = audioClock_.getElapsedTime().asMilliseconds();

    if (now - lastPlayMs < cooldownMs) {
        return;
    }

    sf::Sound* channel = acquireFreeChannel();
    if (!channel) {
        return;
    }

    channel->stop();
    channel->setBuffer(buffer);
    channel->setVolume(volume);
    channel->play();

    lastPlayMs = now;
}

void AudioManager::playEvents(const GameState& state) {
    for (const auto& event : state.events()) {
        switch (event.type()) {
            case EventType::TowerDamaged:
                if (hitLoaded_) {
                    playSound(hitBuffer_, lastHitMs_, 60, 42.0f);
                }
                break;

            case EventType::TowerHealed:
                if (healLoaded_) {
                    playSound(healBuffer_, lastHealMs_, 95, 52.0f);
                }
                break;

            case EventType::TowerDestroyed:
                if (destroyLoaded_) {
                    playSound(destroyBuffer_, lastDestroyMs_, 220, 72.0f);
                }
                break;

            case EventType::UnitSpawned:
                if (spawnLoaded_) {
                    playSound(spawnBuffer_, lastSpawnMs_, 48, 24.0f);
                }
                break;

            default:
                break;
        }
    }
}

} // namespace tw