#include "core/GameConfig.hpp"

namespace TikTokArena
{
    GameConfig createDefaultGameConfig()
    {
        GameConfig config{};

        config.clearColor = Color{12, 12, 18, 255};

        config.arenaRows = 3;
        config.arenaColumns = 5;

        config.arenaGridStyle.outerPaddingRatio = 0.018f;
        config.arenaGridStyle.lineGapRatio = 0.004f;
        config.arenaGridStyle.lineThicknessRatio = 0.0015f;
        config.arenaGridStyle.arenaInsetRatio = 0.008f;
        config.arenaGridStyle.roundness = 0.035f;
        config.arenaGridStyle.roundedSegments = 12;
        config.arenaGridStyle.arenaColor = Color{24, 24, 32, 165};
        config.arenaGridStyle.lineColor = Color{120, 120, 150, 255};

        config.backgroundParticlesStyle.particleCount = 90;
        config.backgroundParticlesStyle.minRadiusRatio = 0.0025f;
        config.backgroundParticlesStyle.maxRadiusRatio = 0.010f;
        config.backgroundParticlesStyle.minSpeedRatio = 0.018f;
        config.backgroundParticlesStyle.maxSpeedRatio = 0.055f;
        config.backgroundParticlesStyle.minAlpha = 45;
        config.backgroundParticlesStyle.maxAlpha = 110;
        config.backgroundParticlesStyle.color = Color{180, 160, 255, 255};

        config.physicsConfig.minCollisionDistance = 0.0001f;
        config.physicsConfig.collisionSeparationBias = 0.5f;
        config.physicsConfig.collisionRestitution = 1.0f;

        config.playerSpawnConfig.testPlayerCount = 50;
        config.playerSpawnConfig.initialHealth = 100.0f;
        config.playerSpawnConfig.radiusRatio = 0.070f;
        config.playerSpawnConfig.speedRatio = 0.60f;
        config.playerSpawnConfig.horizontalSpawnRatio = 0.18f;
        config.playerSpawnConfig.maxInitialAngleOffsetDegrees = 32.0f;

        config.healthBarStyle.widthRatio = 2.8f;
        config.healthBarStyle.heightRatio = 0.35f;
        config.healthBarStyle.offsetRatio = 1.35f;
        config.healthBarStyle.fontSize = 10;
        config.healthBarStyle.mediumHealthThreshold = 0.6f;
        config.healthBarStyle.lowHealthThreshold = 0.3f;
        config.healthBarStyle.backgroundColor = Color{20, 20, 24, 210};
        config.healthBarStyle.borderColor = Color{230, 230, 240, 180};
        config.healthBarStyle.highHealthColor = Color{50, 220, 90, 230};
        config.healthBarStyle.mediumHealthColor = Color{255, 160, 40, 230};
        config.healthBarStyle.lowHealthColor = Color{230, 55, 55, 230};
        config.healthBarStyle.textColor = Color{245, 245, 245, 255};

        config.inputConfig.startDuelsKey = KEY_ENTER;
        config.inputConfig.resetKey = KEY_R;

        return config;
    }
}
