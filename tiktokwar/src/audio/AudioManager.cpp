#include "audio/AudioManager.hpp"
#include <cstdlib>
#include <filesystem>
#include <string>

namespace tw {
namespace {

bool audioDisabledByEnv() {
    const char* enabled = std::getenv("TIKTOK_WAR_AUDIO");
    if (!enabled) {
        return false;
    }

    const std::string value(enabled);
    return value == "0" || value == "false" || value == "FALSE" ||
           value == "off" || value == "OFF";
}

std::filesystem::path resolveSoundDir() {
    const std::filesystem::path soundDir("assets/sounds");
    std::filesystem::path current = std::filesystem::current_path();

    for (int depth = 0; depth < 5; ++depth) {
        const std::filesystem::path candidate = current / soundDir;
        if (std::filesystem::is_directory(candidate)) {
            return candidate;
        }

        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    return soundDir;
}

bool loadSound(sf::SoundBuffer& buffer, const std::filesystem::path& soundDir, const char* fileName) {
    const std::filesystem::path path = soundDir / fileName;
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }

    return buffer.loadFromFile(path.string());
}

} // namespace

AudioManager::AudioManager() {
    audioEnabled_ = !audioDisabledByEnv();
}

bool AudioManager::ensureLoaded() {
    if (!audioEnabled_) {
        return false;
    }
    if (loadAttempted_) {
        return spawnLoaded_ || hitLoaded_ || healLoaded_ || destroyLoaded_;
    }

    loadAttempted_ = true;
    const std::filesystem::path soundDir = resolveSoundDir();
    spawnLoaded_ = loadSound(spawnBuffer_, soundDir, "spawn.wav");
    hitLoaded_ = loadSound(hitBuffer_, soundDir, "hit.wav");
    healLoaded_ = loadSound(healBuffer_, soundDir, "heal.wav");
    destroyLoaded_ = loadSound(destroyBuffer_, soundDir, "destroy.wav");

    if (!spawnLoaded_ && !hitLoaded_ && !healLoaded_ && !destroyLoaded_) {
        audioEnabled_ = false;
        return false;
    }

    channels_.reserve(kChannelCount);
    for (std::size_t i = 0; i < kChannelCount; ++i) {
        auto channel = std::make_unique<sf::Sound>();
        channel->setVolume(100.0f);
        channels_.push_back(std::move(channel));
    }

    return true;
}

sf::Sound* AudioManager::acquireFreeChannel() {
    if (!audioEnabled_) {
        return nullptr;
    }

    for (auto& channel : channels_) {
        if (channel && channel->getStatus() != sf::Sound::Playing) {
            return channel.get();
        }
    }
    return nullptr;
}

void AudioManager::playSound(sf::SoundBuffer& buffer,
                             sf::Int32& lastPlayMs,
                             sf::Int32 cooldownMs,
                             float volume) {
    if (!audioEnabled_) {
        return;
    }

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
    if (!audioEnabled_ || state.events().empty()) {
        return;
    }

    if (!ensureLoaded()) {
        return;
    }

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
