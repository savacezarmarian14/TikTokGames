#include "ui/WaitingPlayersRenderer.hpp"

#include "entities/Weapon.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace TikTokArena
{
    void WaitingPlayersRenderer::draw(
        const std::vector<Player>& waitingPlayers,
        const WeaponSelection& currentSelection,
        const std::vector<Rectangle>& arenas,
        int screenWidth,
        int screenHeight
    ) const
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{0, 0, 0, 78});

        drawHeader(
            screenWidth,
            screenHeight,
            static_cast<int>(waitingPlayers.size()),
            static_cast<int>(arenas.size() * 2U)
        );

        drawCurrentSelection(currentSelection, screenWidth, screenHeight);
        drawWaitingPlayersInArenas(waitingPlayers, arenas, screenWidth, screenHeight);
        drawControls(screenWidth, screenHeight);
    }

    void WaitingPlayersRenderer::drawHeader(
        int screenWidth,
        int screenHeight,
        int playerCount,
        int arenaCapacity
    ) const
    {
        const char* title = "WAITING FOR PLAYERS";
        const int titleFontSize = static_cast<int>(screenHeight * 0.045f);
        const int titleWidth = MeasureText(title, titleFontSize);

        DrawText(
            title,
            screenWidth / 2 - titleWidth / 2,
            static_cast<int>(screenHeight * 0.022f),
            titleFontSize,
            RAYWHITE
        );

        const std::string subtitle =
            "Players ready: " + std::to_string(playerCount) +
            " / " + std::to_string(arenaCapacity) +
            "    Max 2 players per arena";

        const int subtitleFontSize = static_cast<int>(screenHeight * 0.021f);
        const int subtitleWidth = MeasureText(subtitle.c_str(), subtitleFontSize);

        DrawText(
            subtitle.c_str(),
            screenWidth / 2 - subtitleWidth / 2,
            static_cast<int>(screenHeight * 0.073f),
            subtitleFontSize,
            Color{210, 210, 225, 255}
        );
    }

    void WaitingPlayersRenderer::drawControls(int screenWidth, int screenHeight) const
    {
        const char* controls = "1 = add Bat    2 = add Laser    3 = add Gun    SPACE = spawn player    ENTER = start duels    R = reset";
        const int fontSize = static_cast<int>(screenHeight * 0.020f);
        const int textWidth = MeasureText(controls, fontSize);

        DrawText(
            controls,
            screenWidth / 2 - textWidth / 2,
            static_cast<int>(screenHeight * 0.956f),
            fontSize,
            Color{220, 220, 235, 255}
        );
    }

    void WaitingPlayersRenderer::drawCurrentSelection(
        const WeaponSelection& currentSelection,
        int screenWidth,
        int screenHeight
    ) const
    {
        const Rectangle panel{
            screenWidth * 0.5f - screenWidth * 0.17f,
            screenHeight * 0.105f,
            screenWidth * 0.34f,
            screenHeight * 0.095f
        };

        DrawRectangleRounded(panel, 0.18f, 12, Color{20, 20, 30, 210});
        DrawRectangleRoundedLinesEx(panel, 0.18f, 12, 2.0f, Color{190, 190, 220, 230});

        const char* title = "Current player weapons";
        const int titleFontSize = static_cast<int>(panel.height * 0.23f);
        const int titleWidth = MeasureText(title, titleFontSize);

        DrawText(
            title,
            static_cast<int>(panel.x + panel.width * 0.5f - titleWidth * 0.5f),
            static_cast<int>(panel.y + panel.height * 0.11f),
            titleFontSize,
            RAYWHITE
        );

        const std::vector<std::string> lines = buildSelectionLines(currentSelection);

        std::string inlineText;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (i > 0)
            {
                inlineText += "   ";
            }

            inlineText += lines[i];
        }

        const int weaponFontSize = static_cast<int>(panel.height * 0.23f);
        const int weaponTextWidth = MeasureText(inlineText.c_str(), weaponFontSize);

        DrawText(
            inlineText.c_str(),
            static_cast<int>(panel.x + panel.width * 0.5f - weaponTextWidth * 0.5f),
            static_cast<int>(panel.y + panel.height * 0.56f),
            weaponFontSize,
            Color{235, 235, 245, 255}
        );
    }

    void WaitingPlayersRenderer::drawWaitingPlayersInArenas(
        const std::vector<Player>& waitingPlayers,
        const std::vector<Rectangle>& arenas,
        int screenWidth,
        int screenHeight
    ) const
    {
        if (waitingPlayers.empty())
        {
            const char* hint = "Press 1 / 2 to choose weapons, then SPACE to add the first player.";
            const int fontSize = static_cast<int>(screenHeight * 0.03f);
            const int textWidth = MeasureText(hint, fontSize);

            DrawText(
                hint,
                screenWidth / 2 - textWidth / 2,
                static_cast<int>(screenHeight * 0.50f),
                fontSize,
                Color{230, 230, 245, 255}
            );

            return;
        }

        for (size_t playerIndex = 0; playerIndex < waitingPlayers.size(); ++playerIndex)
        {
            const size_t arenaIndex = playerIndex / 2U;

            if (arenaIndex >= arenas.size())
            {
                break;
            }

            const int slotIndex = static_cast<int>(playerIndex % 2U);
            drawArenaWaitingSlot(waitingPlayers[playerIndex], arenas[arenaIndex], slotIndex, 2);
        }

        if (waitingPlayers.size() > arenas.size() * 2U)
        {
            const std::string warning =
                "+" + std::to_string(waitingPlayers.size() - arenas.size() * 2U) +
                " players not visible - increase arena count";

            const int fontSize = static_cast<int>(screenHeight * 0.024f);
            const int textWidth = MeasureText(warning.c_str(), fontSize);

            DrawText(
                warning.c_str(),
                screenWidth / 2 - textWidth / 2,
                static_cast<int>(screenHeight * 0.915f),
                fontSize,
                Color{255, 190, 80, 255}
            );
        }
    }

    void WaitingPlayersRenderer::drawArenaWaitingSlot(
        const Player& player,
        const Rectangle& arena,
        int slotIndex,
        int slotCount
    ) const
    {
        const float marginX = arena.width * 0.06f;
        const float marginY = arena.height * 0.13f;
        const float gap = arena.width * 0.04f;

        const float slotWidth =
            (arena.width - 2.0f * marginX - gap * static_cast<float>(slotCount - 1)) /
            static_cast<float>(slotCount);

        const Rectangle slot{
            arena.x + marginX + static_cast<float>(slotIndex) * (slotWidth + gap),
            arena.y + marginY,
            slotWidth,
            arena.height - 2.0f * marginY
        };

        DrawRectangleRounded(slot, 0.20f, 10, Color{15, 15, 24, 185});
        DrawRectangleRoundedLinesEx(slot, 0.20f, 10, 1.5f, Color{210, 210, 230, 210});

        const int nameFontSize = static_cast<int>(std::min(slot.width * 0.13f, slot.height * 0.11f));
        const int nameWidth = MeasureText(player.username.c_str(), nameFontSize);

        DrawText(
            player.username.c_str(),
            static_cast<int>(slot.x + slot.width * 0.5f - static_cast<float>(nameWidth) * 0.5f),
            static_cast<int>(slot.y + slot.height * 0.08f),
            nameFontSize,
            RAYWHITE
        );

        const float avatarRadius = std::min(slot.width, slot.height) * 0.17f;
        const Vector2 avatar{
            slot.x + slot.width * 0.5f,
            slot.y + slot.height * 0.34f
        };

        DrawCircleV(avatar, avatarRadius, player.color);
        DrawCircleLines(
            static_cast<int>(avatar.x),
            static_cast<int>(avatar.y),
            avatarRadius,
            RAYWHITE
        );

        const Rectangle weaponBox{
            slot.x + slot.width * 0.08f,
            slot.y + slot.height * 0.56f,
            slot.width * 0.84f,
            slot.height * 0.36f
        };

        DrawRectangleRounded(weaponBox, 0.18f, 8, Color{0, 0, 0, 75});
        DrawRectangleRoundedLinesEx(weaponBox, 0.18f, 8, 1.0f, Color{210, 210, 225, 180});

        drawCompactWeaponList(
            player,
            weaponBox,
            static_cast<int>(std::min(slot.width * 0.105f, slot.height * 0.085f))
        );
    }

    void WaitingPlayersRenderer::drawCompactWeaponList(
        const Player& player,
        Rectangle box,
        int fontSize
    ) const
    {
        drawWeaponLines(buildWeaponLines(player), box, fontSize);
    }

    void WaitingPlayersRenderer::drawWeaponLines(
        const std::vector<std::string>& lines,
        Rectangle box,
        int fontSize
    ) const
    {
        const int safeFontSize = std::max(8, fontSize);
        const int lineSpacing = safeFontSize + 3;
        const int totalTextHeight = static_cast<int>(lines.size()) * lineSpacing;

        int y = static_cast<int>(
            box.y + box.height * 0.5f - static_cast<float>(totalTextHeight) * 0.5f
        );

        for (const std::string& line : lines)
        {
            const int textWidth = MeasureText(line.c_str(), safeFontSize);
            const int x = static_cast<int>(
                box.x + box.width * 0.5f - static_cast<float>(textWidth) * 0.5f
            );

            DrawText(line.c_str(), x, y, safeFontSize, RAYWHITE);
            y += lineSpacing;
        }
    }

    std::vector<std::string> WaitingPlayersRenderer::buildWeaponLines(const Player& player) const
    {
        int batCount = 0;
        int laserCount = 0;
        int gunCount = 0;
        int basicCount = 0;

        for (const auto& weapon : player.weapons)
        {
            if (!weapon)
            {
                continue;
            }

            const std::string& id = weapon->getId();

            if (id == "bat_weapon")
            {
                ++batCount;
            }
            else if (id == "laser_weapon")
            {
                ++laserCount;
            }
            else if (id == "gun_weapon")
            {
                ++gunCount;
            }
            else if (id == "basic_weapon")
            {
                ++basicCount;
            }
        }

        std::vector<std::string> lines;

        if (batCount > 0)
        {
            lines.push_back(std::to_string(batCount) + "xBat");
        }

        if (laserCount > 0)
        {
            lines.push_back(std::to_string(laserCount) + "xLaser");
        }

        if (gunCount > 0)
        {
            lines.push_back(std::to_string(gunCount) + "xGun");
        }

        if (basicCount > 0)
        {
            lines.push_back(std::to_string(basicCount) + "xBasic");
        }

        return lines;
    }

    std::vector<std::string> WaitingPlayersRenderer::buildSelectionLines(
        const WeaponSelection& currentSelection
    ) const
    {
        std::vector<std::string> lines;

        if (currentSelection.getBatCount() > 0)
        {
            lines.push_back(std::to_string(currentSelection.getBatCount()) + "xBat");
        }

        if (currentSelection.getLaserCount() > 0)
        {
            lines.push_back(std::to_string(currentSelection.getLaserCount()) + "xLaser");
        }

        if (currentSelection.getGunCount() > 0)
        {
            lines.push_back(std::to_string(currentSelection.getGunCount()) + "xGun");
        }

        lines.push_back(std::to_string(currentSelection.getBasicAttackCount()) + "xBasic Attack");

        return lines;
    }
}
