#ifndef POWER_UP_HPP
#define POWER_UP_HPP

#include <string>

namespace TikTokArena
{
    class PowerUp
    {
    public:
        PowerUp(std::string id, std::string name);
        virtual ~PowerUp() = default;

        const std::string& getId() const;
        const std::string& getName() const;
        virtual void apply() = 0;

    private:
        std::string id_;
        std::string name_;
    };
}

#endif
