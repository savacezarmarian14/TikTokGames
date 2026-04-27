#include "physics/WallCollisionResolver.hpp"

namespace TikTokArena
{
    void WallCollisionResolver::resolve(Player& player, const Rectangle& bounds) const
    {
        const float left = bounds.x + player.radius;
        const float right = bounds.x + bounds.width - player.radius;
        const float top = bounds.y + player.radius;
        const float bottom = bounds.y + bounds.height - player.radius;

        if (player.position.x < left)
        {
            player.position.x = left;
            player.velocity.x *= -1.0f;
        }
        else if (player.position.x > right)
        {
            player.position.x = right;
            player.velocity.x *= -1.0f;
        }

        if (player.position.y < top)
        {
            player.position.y = top;
            player.velocity.y *= -1.0f;
        }
        else if (player.position.y > bottom)
        {
            player.position.y = bottom;
            player.velocity.y *= -1.0f;
        }
    }
}
