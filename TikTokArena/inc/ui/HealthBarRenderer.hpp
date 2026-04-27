#ifndef HEALTH_BAR_RENDERER_HPP
#define HEALTH_BAR_RENDERER_HPP

#include "raylib.h"

namespace TikTokArena
{
    struct HealthBarStyle
    {
        float widthRatio;
        float heightRatio;
        float offsetRatio;

        int fontSize;

        float mediumHealthThreshold;
        float lowHealthThreshold;

        Color backgroundColor;
        Color borderColor;

        Color highHealthColor;
        Color mediumHealthColor;
        Color lowHealthColor;

        Color textColor;
    };

    class HealthBarRenderer
    {
    public:
        explicit HealthBarRenderer(const HealthBarStyle& style);

        void draw(
            const Vector2& playerPosition,
            float playerRadius,
            float health,
            float maxHealth
        ) const;

    private:
        Color getFillColor(float healthRatio) const;

    private:
        HealthBarStyle style_;
    };
}

#endif