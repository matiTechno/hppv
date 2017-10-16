#pragma once

#include <memory>

#include <glm/vec2.hpp>

namespace hppv
{

struct Frame;

class Renderer;

class Scene
{
public:
    Scene();

    virtual ~Scene() = default;

    virtual void processInput(bool hasInput);

    virtual void update();

    virtual void render(Renderer& renderer);

    struct Properties
    {
        glm::ivec2 pos = {0, 0};
        glm::ivec2 size = {100, 100};
        bool maximize = false;
        bool opaque = true;
        // only polled for the top scene
        unsigned numScenesToPop = 0;
        std::unique_ptr<Scene> sceneToPush;
    } properties_;

    const Frame& frame_;
};

} // namespace hppv
