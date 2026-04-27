#ifndef ARENA_GRID_HPP
#define ARENA_GRID_HPP

#include "raylib.h"

#include <vector>

namespace TikTokArena
{
    struct ArenaGridStyle
    {
        float outerPaddingRatio;
        float lineGapRatio;
        float lineThicknessRatio;
        float arenaInsetRatio;
        float roundness;
        int roundedSegments;
        Color arenaColor;
        Color lineColor;
    };

    class ArenaGrid
    {
    public:
        ArenaGrid(int rows, int columns, ArenaGridStyle style);

        void updateLayout(int screenWidth, int screenHeight);
        void draw() const;

        const std::vector<Rectangle>& getArenas() const;
        int getArenaCount() const;

    private:
        void updateScaledStyle();
        void calculateArenaBounds();
        void drawArenaBackgrounds() const;
        void drawGridLines() const;
        void drawVerticalDoubleLine(float x, float top, float bottom) const;
        void drawHorizontalDoubleLine(float y, float left, float right) const;

    private:
        int rows_{};
        int columns_{};
        int screenWidth_{};
        int screenHeight_{};
        ArenaGridStyle style_{};
        float outerPadding_{};
        float lineGap_{};
        float lineThickness_{};
        float arenaInset_{};
        std::vector<Rectangle> arenas_;
    };
}

#endif
