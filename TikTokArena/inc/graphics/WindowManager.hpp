#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

#include <string>

namespace TikTokArena
{
    class WindowManager
    {
    public:
        WindowManager() = default;
        ~WindowManager();

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;

        void createFullscreenWindow(const std::string& title);
        bool shouldClose() const;

        int getWidth() const;
        int getHeight() const;

    private:
        struct MonitorInfo
        {
            int index{};
            int width{};
            int height{};

            int area() const;
        };

        MonitorInfo findBestMonitor() const;

    private:
        bool isWindowCreated_{false};
    };
}

#endif
