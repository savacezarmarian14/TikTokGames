#include "graphics/WindowManager.hpp"

#include "raylib.h"

#include <stdexcept>
#include <iostream>

namespace TikTokArena
{
    WindowManager::~WindowManager()
    {
        if (isWindowCreated_)
        {
            CloseWindow();
        }
    }

    int WindowManager::MonitorInfo::area() const
    {
        return width * height;
    }

    WindowManager::MonitorInfo WindowManager::findBestMonitor() const
    {
        const int monitorCount = GetMonitorCount();
        std::cout << "[DEBUG] " << monitorCount << std::endl;

        if (monitorCount <= 0)
        {
            throw std::runtime_error("No monitors detected.");
        }

        MonitorInfo bestMonitor{0, GetMonitorWidth(0), GetMonitorHeight(0)};

        for (int index = 1; index < monitorCount; ++index)
        {
            const MonitorInfo currentMonitor{index, GetMonitorWidth(index), GetMonitorHeight(index)};

            if (currentMonitor.area() > bestMonitor.area())
            {
                bestMonitor = currentMonitor;
            }
        }
        return MonitorInfo{1, GetMonitorWidth(1), GetMonitorHeight(1)};
        return bestMonitor;
    }

    void WindowManager::createFullscreenWindow(const std::string& title)
    {
        if (isWindowCreated_)
        {
            throw std::runtime_error("Window was already created.");
        }

        SetConfigFlags(FLAG_WINDOW_UNDECORATED);
        InitWindow(1, 1, title.c_str());

        const MonitorInfo bestMonitor = findBestMonitor();

        SetWindowMonitor(bestMonitor.index);
        SetWindowSize(bestMonitor.width, bestMonitor.height);
        ToggleFullscreen();
        SetTargetFPS(GetMonitorRefreshRate(bestMonitor.index));

        isWindowCreated_ = true;
    }

    bool WindowManager::shouldClose() const
    {
        return WindowShouldClose();
    }

    int WindowManager::getWidth() const
    {
        return GetScreenWidth();
    }

    int WindowManager::getHeight() const
    {
        return GetScreenHeight();
    }
}
