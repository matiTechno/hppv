#pragma once

#include <vector>

#include <hppv/Scene.hpp>
#include <hppv/Space.hpp>

namespace hppv
{
class Font;
class Texture;
}

class Snake: public hppv::Scene
{
public:
    Snake();

    void processInput(bool hasInput) override;

    void update() override;

    void render(hppv::Renderer& renderer) override;

private:
    enum {MapSize = 15};

    float accumulator = 0.f;
    std::vector<glm::ivec2> nodes_;
    std::vector<glm::vec2> freeTiles_;
    static inline hppv::Space space_ = {0.f, 0.f, 100.f, 100.f};
    static inline float timeStep_ = 0.3f;

    struct Food
    {
        static inline float spawnTime = 6.f;
        float timeLeft;
        glm::ivec2 pos;
    }
    food_;

    struct
    {
        glm::ivec2 current;
        glm::ivec2 next;
    }
    vel_;
};
