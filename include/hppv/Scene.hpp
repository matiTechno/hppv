#pragma once

#include <memory>

#include <glm/vec2.hpp>

#include "Frame.hpp"

namespace hppv
{

class Renderer;

class Scene
{
public:
    Scene();

    virtual ~Scene() = default;

    virtual void processInput(bool hasInput) {(void)hasInput;}

    virtual void update() {}

    virtual void render(Renderer& renderer) {(void)renderer;}

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
    properties_;

    const Frame& frame_;
};

// todo?: find better place for this?

template<typename T, int N>
constexpr int size(T(&)[N])
{
    return N;
}

} // namespace hppv
