#pragma once

#include <vector>

#include <hppv/Scene.hpp>
#include <hppv/Space.hpp>

class Snake: public hppv::Scene
{
public:
    Snake();

    void processInput(bool hasInput) override;

    void update() override;

    void render(hppv::Renderer& renderer) override;

private:
    enum {MapSize = 20};

    static inline float timestep_ = 0.0888f;
    float accumulator_ = 0.f;
    std::vector<glm::ivec2> nodes_;

    struct Food
    {
        static inline float spawnTime = 5.f;
        float timeLeft = spawnTime;
        glm::ivec2 pos;
    }
    food_;

    struct
    {
        glm::ivec2 current = {0, -1};
        glm::ivec2 next = current;
    }
    vel_;

    struct NewNode
    {
        glm::ivec2 pos;
        int turnsToSpawn;
    };

    std::vector<NewNode> newNodes_;
};
