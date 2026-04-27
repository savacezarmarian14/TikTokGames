#include "ui/ArenaGrid.hpp"

#include <algorithm>
#include <stdexcept>

namespace TikTokArena
{
    ArenaGrid::ArenaGrid(int rows, int columns, ArenaGridStyle style)
        : rows_(rows),
          columns_(columns),
          style_(style)
    {
        if (rows_ <= 0 || columns_ <= 0)
        {
            throw std::invalid_argument("ArenaGrid rows and columns must be positive.");
        }

        arenas_.reserve(static_cast<size_t>(rows_ * columns_));
    }

    void ArenaGrid::updateLayout(int screenWidth, int screenHeight)
    {
        screenWidth_ = screenWidth;
        screenHeight_ = screenHeight;
        updateScaledStyle();
        calculateArenaBounds();
    }

    void ArenaGrid::updateScaledStyle()
    {
        const float baseUnit = static_cast<float>(std::min(screenWidth_, screenHeight_));
        outerPadding_ = baseUnit * style_.outerPaddingRatio;
        lineGap_ = baseUnit * style_.lineGapRatio;
        lineThickness_ = baseUnit * style_.lineThicknessRatio;
        arenaInset_ = baseUnit * style_.arenaInsetRatio;
    }

    void ArenaGrid::calculateArenaBounds()
    {
        arenas_.clear();

        const float left = outerPadding_;
        const float top = outerPadding_;
        const float right = static_cast<float>(screenWidth_) - outerPadding_;
        const float bottom = static_cast<float>(screenHeight_) - outerPadding_;
        const float gridWidth = right - left;
        const float gridHeight = bottom - top;
        const float cellWidth = gridWidth / static_cast<float>(columns_);
        const float cellHeight = gridHeight / static_cast<float>(rows_);

        for (int row = 0; row < rows_; ++row)
        {
            for (int column = 0; column < columns_; ++column)
            {
                const float x = left + static_cast<float>(column) * cellWidth;
                const float y = top + static_cast<float>(row) * cellHeight;
                arenas_.push_back(Rectangle{x + arenaInset_, y + arenaInset_, cellWidth - 2.0f * arenaInset_, cellHeight - 2.0f * arenaInset_});
            }
        }
    }

    void ArenaGrid::draw() const
    {
        drawArenaBackgrounds();
        drawGridLines();
    }

    void ArenaGrid::drawArenaBackgrounds() const
    {
        for (const Rectangle& arena : arenas_)
        {
            DrawRectangleRounded(arena, style_.roundness, style_.roundedSegments, style_.arenaColor);
        }
    }

    void ArenaGrid::drawGridLines() const
    {
        const float left = outerPadding_;
        const float top = outerPadding_;
        const float right = static_cast<float>(screenWidth_) - outerPadding_;
        const float bottom = static_cast<float>(screenHeight_) - outerPadding_;
        const float gridWidth = right - left;
        const float gridHeight = bottom - top;
        const float cellWidth = gridWidth / static_cast<float>(columns_);
        const float cellHeight = gridHeight / static_cast<float>(rows_);

        for (int column = 0; column <= columns_; ++column)
        {
            drawVerticalDoubleLine(left + static_cast<float>(column) * cellWidth, top, bottom);
        }

        for (int row = 0; row <= rows_; ++row)
        {
            drawHorizontalDoubleLine(top + static_cast<float>(row) * cellHeight, left, right);
        }
    }

    void ArenaGrid::drawVerticalDoubleLine(float x, float top, float bottom) const
    {
        const float halfGap = lineGap_ * 0.5f;
        DrawRectangleRec(Rectangle{x - halfGap - lineThickness_, top, lineThickness_, bottom - top}, style_.lineColor);
        DrawRectangleRec(Rectangle{x + halfGap, top, lineThickness_, bottom - top}, style_.lineColor);
    }

    void ArenaGrid::drawHorizontalDoubleLine(float y, float left, float right) const
    {
        const float halfGap = lineGap_ * 0.5f;
        DrawRectangleRec(Rectangle{left, y - halfGap - lineThickness_, right - left, lineThickness_}, style_.lineColor);
        DrawRectangleRec(Rectangle{left, y + halfGap, right - left, lineThickness_}, style_.lineColor);
    }

    const std::vector<Rectangle>& ArenaGrid::getArenas() const
    {
        return arenas_;
    }

    int ArenaGrid::getArenaCount() const
    {
        return rows_ * columns_;
    }
}
