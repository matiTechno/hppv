#pragma once

#include <random>
#include <vector>

#include <hppv/Renderer.hpp>

template<typename T>
struct Range
{
    T min;
    T max;
};

class Emitter
{
public:
    using Distribution = std::uniform_real_distribution<float>;

    Emitter(std::mt19937* generator): generator(generator) {}
    Emitter(std::mt19937& generator): generator(&generator) {}

    void reserveMemory(); // based on life and spawn.hz

    void update(float frameTime);

    void render(hppv::Renderer& renderer);

    std::size_t getCount() const {return count_;}

    std::mt19937* generator;

    struct
    {
        glm::vec2 pos;
        glm::vec2 size;
        std::size_t hz;
    }
    spawn;

    Range<float> life;
    Range<glm::vec2> vel;
    Range<glm::vec2> acc;
    Range<float> radius;
    Range<glm::vec4> colorStart;
    Range<glm::vec4> colorEnd;

private:
    struct Particle
    {
        float life;
        glm::vec4 colorVel;
        glm::vec2 vel;
        glm::vec2 acc;
    };

    std::vector<Particle> particles_;
    std::vector<hppv::Circle> circles_;
    std::size_t count_ = 0;
    float accumulator_ = 0.f;

    void killParticle(std::size_t index);
    void spawnParticle();
};
