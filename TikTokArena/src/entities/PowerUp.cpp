#include "entities/PowerUp.hpp"

#include <utility>

namespace TikTokArena
{
    PowerUp::PowerUp(std::string id, std::string name)
        : id_(std::move(id)), name_(std::move(name))
    {
    }

    const std::string& PowerUp::getId() const { return id_; }
    const std::string& PowerUp::getName() const { return name_; }
}
