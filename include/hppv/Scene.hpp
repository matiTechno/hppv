#pragma once

#include <memory> // std::unique_ptr
#include <vector>

#include <glm/vec2.hpp>

#include "Frame.hpp"
#include "Event.hpp"
// included for convenience
#include "widgets.hpp"

namespace hppv
{

class Renderer;

class Scene
{
public:
    Scene();

    virtual ~Scene() = default;

    // called every frame on the top scene
    virtual void processInput(const std::vector<Event>&) {}

    virtual void update() {}

    // App: renderer.viewport(scene);
    virtual void render(Renderer&) {}
    // App: renderer.flush();

    struct
    {
        glm::ivec2 pos = {0, 0};
        glm::ivec2 size = {100, 100};
        bool maximize = false;
        bool opaque = true;
        bool updateWhenNotTop = false;
        // only polled for the top scene
        unsigned numScenesToPop = 0;
        std::unique_ptr<Scene> sceneToPush;
    }
    properties_; // note: convention exception

    const Frame& frame_; // same
};

} // namespace hppv
