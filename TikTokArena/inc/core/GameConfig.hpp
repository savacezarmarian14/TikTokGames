#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

#include "effects/BackgroundParticles.hpp"
#include "physics/PhysicsConfig.hpp"
#include "ui/HealthBarRenderer.hpp"
#include "raylib.h"
#include "ui/ArenaGrid.hpp"

#include <string>

namespace TikTokArena
{
    struct PlayerSpawnConfig
    {
        int testPlayerCount;
        float initialHealth;
        float radiusRatio;
        float speedRatio;
        float horizontalSpawnRatio;
        float maxInitialAngleOffsetDegrees;
    };

    struct InputConfig
    {
        int startDuelsKey;
        int resetKey;
    };

    struct GameConfig
    {
        Color clearColor;

        int arenaRows;
        int arenaColumns;

        ArenaGridStyle arenaGridStyle;
        BackgroundParticlesStyle backgroundParticlesStyle;
        PhysicsConfig physicsConfig;
        PlayerSpawnConfig playerSpawnConfig;
        InputConfig inputConfig;
        HealthBarStyle healthBarStyle;
    };

    GameConfig createDefaultGameConfig();
}

#endif
