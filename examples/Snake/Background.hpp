#pragma once

#include <random>

#include <hppv/Scene.hpp>
#include <hppv/Font.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Texture.hpp>

struct Resources
{
    hppv::Font font;
    hppv::Texture texSnakeFood;
    std::mt19937 rng;
};

class Background: public hppv::Scene
{
public:
    Background();

    void processInput(bool hasInput) override;

    void update() override;

    void render(hppv::Renderer& renderer) override;

    static Resources* resources;

private:
    hppv::Shader shWave_;
    float time_ = 0.f;
    Resources resources_;

    struct Animation
    {
        int count = 0;
        float accumulator = 0.f;
        static inline float timeStep = 0.5f;
    }
    animation_;
};
