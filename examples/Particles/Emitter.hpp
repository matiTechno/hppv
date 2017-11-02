#pragma once

#include <random>
#include <vector>

#include <hppv/Renderer.hpp>

template<typename T>
struct MinMax
{
    T min;
    T max;
};

class Emitter
{
public:
    using Distribution = std::uniform_real_distribution<float>;

    void update(float frameTime);

    void render(hppv::Renderer& renderer);

    std::mt19937* generator;

    struct
    {
        glm::vec2 pos;
        glm::vec2 size;
        std::size_t hz;
    }
    spawn;

    MinMax<float> life;
    MinMax<glm::vec2> vel;
    MinMax<glm::vec2> acc;
    MinMax<float> radius;
    MinMax<glm::vec4> colorStart;
    MinMax<glm::vec4> colorEnd;

    std::size_t getCount() const {return count;}

private:
    struct Particle
    {
        float life;
        glm::vec4 colorVel;
        glm::vec2 vel;
        glm::vec2 acc;
    };

    std::vector<Particle> particles;
    std::vector<hppv::Circle> circles;
    std::size_t count = 0;
    float accumulator = 0.f;

    void killP(std::size_t index);
    void spawnP();
};
