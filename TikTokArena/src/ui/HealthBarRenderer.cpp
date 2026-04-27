#include "ui/HealthBarRenderer.hpp"

#include <algorithm>
#include <string>

namespace TikTokArena
{
    HealthBarRenderer::HealthBarRenderer(const HealthBarStyle& style)
        : style_(style)
    {
    }

    void HealthBarRenderer::draw(
        const Vector2& playerPosition,
        float playerRadius,
        float health,
        float maxHealth
    ) const
    {
        if (maxHealth <= 0.0f)
        {
            return;
        }

        const float healthRatio = std::clamp(health / maxHealth, 0.0f, 1.0f);

        const float width = playerRadius * style_.widthRatio;
        const float height = playerRadius * style_.heightRatio;

        const float x = playerPosition.x - width * 0.5f;
        const float y = playerPosition.y + playerRadius * style_.offsetRatio;

        const Rectangle background{x, y, width, height};
        const Rectangle fill{x, y, width * healthRatio, height};

        DrawRectangleRounded(background, 0.35f, 8, style_.backgroundColor);
        DrawRectangleRounded(fill, 0.35f, 8, getFillColor(healthRatio));
        DrawRectangleRoundedLines(background, 0.35f, 8, style_.borderColor);

        const std::string text =
            std::to_string(static_cast<int>(health)) + "/" +
            std::to_string(static_cast<int>(maxHealth));

        const int textWidth = MeasureText(text.c_str(), style_.fontSize);

        DrawText(
            text.c_str(),
            static_cast<int>(playerPosition.x - static_cast<float>(textWidth) * 0.5f),
            static_cast<int>(y + height * 0.5f - static_cast<float>(style_.fontSize) * 0.5f),
            style_.fontSize,
            style_.textColor
        );
    }

    Color HealthBarRenderer::getFillColor(float healthRatio) const
    {
        if (healthRatio <= style_.lowHealthThreshold)
        {
            return style_.lowHealthColor;
        }

        if (healthRatio <= style_.mediumHealthThreshold)
        {
            return style_.mediumHealthColor;
        }

        return style_.highHealthColor;
    }
}