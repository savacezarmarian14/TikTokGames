#ifndef WAITING_PLAYERS_RENDERER_HPP
#define WAITING_PLAYERS_RENDERER_HPP

#include "entities/Player.hpp"
#include "gameplay/WeaponSelection.hpp"
#include "raylib.h"

#include <string>
#include <vector>

namespace TikTokArena
{
    class WaitingPlayersRenderer
    {
    public:
        void draw(
            const std::vector<Player>& waitingPlayers,
            const WeaponSelection& currentSelection,
            const std::vector<Rectangle>& arenas,
            int screenWidth,
            int screenHeight
        ) const;

    private:
        void drawHeader(int screenWidth, int screenHeight, int playerCount, int arenaCapacity) const;
        void drawControls(int screenWidth, int screenHeight) const;
        void drawCurrentSelection(const WeaponSelection& currentSelection, int screenWidth, int screenHeight) const;

        void drawWaitingPlayersInArenas(
            const std::vector<Player>& waitingPlayers,
            const std::vector<Rectangle>& arenas,
            int screenWidth,
            int screenHeight
        ) const;

        void drawArenaWaitingSlot(
            const Player& player,
            const Rectangle& arena,
            int slotIndex,
            int slotCount
        ) const;

        void drawCompactWeaponList(const Player& player, Rectangle box, int fontSize) const;
        void drawWeaponLines(const std::vector<std::string>& lines, Rectangle box, int fontSize) const;

        std::vector<std::string> buildWeaponLines(const Player& player) const;
        std::vector<std::string> buildSelectionLines(const WeaponSelection& currentSelection) const;
    };
}

#endif
