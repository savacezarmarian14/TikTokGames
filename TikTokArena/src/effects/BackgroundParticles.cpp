#include "effects/BackgroundParticles.hpp"

#include <algorithm>
#include <cmath>

namespace TikTokArena
{
    BackgroundParticles::BackgroundParticles(BackgroundParticlesStyle style)
        : style_(style)
    {
    }

    void BackgroundParticles::initialize(int screenWidth, int screenHeight)
    {
        lastScreenWidth_ = screenWidth;
        lastScreenHeight_ = screenHeight;
        rebuild(screenWidth, screenHeight);
        initialized_ = true;
    }

    void BackgroundParticles::update(float deltaTime, int screenWidth, int screenHeight)
    {
        if (!initialized_ || screenWidth != lastScreenWidth_ || screenHeight != lastScreenHeight_)
        {
            initialize(screenWidth, screenHeight);
            return;
        }

        for (Particle& particle : particles_)
        {
            particle.position.x += particle.velocity.x * deltaTime;
            particle.position.y += particle.velocity.y * deltaTime;

            const float margin = particle.radius * 2.0f;
            const bool outside = particle.position.x < -margin || particle.position.x > static_cast<float>(screenWidth) + margin ||
                                 particle.position.y < -margin || particle.position.y > static_cast<float>(screenHeight) + margin;

            if (outside)
            {
                particle = createParticleFromEdge(screenWidth, screenHeight);
            }
        }
    }

    void BackgroundParticles::draw() const
    {
        for (const Particle& particle : particles_)
        {
            Color color = style_.color;
            color.a = particle.alpha;
            DrawCircleV(particle.position, particle.radius, color);
        }
    }

    void BackgroundParticles::rebuild(int screenWidth, int screenHeight)
    {
        particles_.clear();
        particles_.reserve(static_cast<size_t>(style_.particleCount));

        for (int index = 0; index < style_.particleCount; ++index)
        {
            particles_.push_back(createParticle(screenWidth, screenHeight));
        }
    }

    BackgroundParticles::Particle BackgroundParticles::createParticle(int screenWidth, int screenHeight) const
    {
        const float baseUnit = static_cast<float>(std::min(screenWidth, screenHeight));
        const float radius = randomFloat(baseUnit * style_.minRadiusRatio, baseUnit * style_.maxRadiusRatio);
        const float speed = randomFloat(baseUnit * style_.minSpeedRatio, baseUnit * style_.maxSpeedRatio);
        const float angle = randomFloat(0.0f, 2.0f * PI);

        return Particle{Vector2{randomFloat(0.0f, static_cast<float>(screenWidth)), randomFloat(0.0f, static_cast<float>(screenHeight))},
                        Vector2{std::cos(angle) * speed, std::sin(angle) * speed},
                        radius,
                        randomAlpha()};
    }

    BackgroundParticles::Particle BackgroundParticles::createParticleFromEdge(int screenWidth, int screenHeight) const
    {
        Particle particle = createParticle(screenWidth, screenHeight);
        const int edge = GetRandomValue(0, 3);

        if (edge == 0) particle.position.x = -particle.radius;
        else if (edge == 1) particle.position.x = static_cast<float>(screenWidth) + particle.radius;
        else if (edge == 2) particle.position.y = -particle.radius;
        else particle.position.y = static_cast<float>(screenHeight) + particle.radius;

        const Vector2 center{static_cast<float>(screenWidth) * 0.5f, static_cast<float>(screenHeight) * 0.5f};
        Vector2 direction{center.x - particle.position.x, center.y - particle.position.y};
        const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (length > 0.0f)
        {
            direction.x /= length;
            direction.y /= length;
        }

        const float baseUnit = static_cast<float>(std::min(screenWidth, screenHeight));
        const float speed = randomFloat(baseUnit * style_.minSpeedRatio, baseUnit * style_.maxSpeedRatio);
        particle.velocity = Vector2{direction.x * speed, direction.y * speed};

        return particle;
    }

    float BackgroundParticles::randomFloat(float min, float max) const
    {
        return min + static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f * (max - min);
    }

    unsigned char BackgroundParticles::randomAlpha() const
    {
        return static_cast<unsigned char>(GetRandomValue(static_cast<int>(style_.minAlpha), static_cast<int>(style_.maxAlpha)));
    }
}
