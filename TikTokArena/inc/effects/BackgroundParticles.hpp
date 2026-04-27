#ifndef BACKGROUND_PARTICLES_HPP
#define BACKGROUND_PARTICLES_HPP

#include "raylib.h"

#include <vector>

namespace TikTokArena
{
    struct BackgroundParticlesStyle
    {
        int particleCount;
        float minRadiusRatio;
        float maxRadiusRatio;
        float minSpeedRatio;
        float maxSpeedRatio;
        unsigned char minAlpha;
        unsigned char maxAlpha;
        Color color;
    };

    class BackgroundParticles
    {
    public:
        explicit BackgroundParticles(BackgroundParticlesStyle style);

        void initialize(int screenWidth, int screenHeight);
        void update(float deltaTime, int screenWidth, int screenHeight);
        void draw() const;

    private:
        struct Particle
        {
            Vector2 position{};
            Vector2 velocity{};
            float radius{};
            unsigned char alpha{};
        };

        void rebuild(int screenWidth, int screenHeight);
        Particle createParticle(int screenWidth, int screenHeight) const;
        Particle createParticleFromEdge(int screenWidth, int screenHeight) const;
        float randomFloat(float min, float max) const;
        unsigned char randomAlpha() const;

    private:
        BackgroundParticlesStyle style_{};
        std::vector<Particle> particles_;
        int lastScreenWidth_{};
        int lastScreenHeight_{};
        bool initialized_{false};
    };
}

#endif
