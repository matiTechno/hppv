#pragma once

#include <memory>

#include "Rect.hpp"

namespace hppv
{

class Renderer;
struct Frame;

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
        Rect rect = {{0.f, 0.f}, {100.f, 100.f}};
        bool maximize = false;
        bool opaque = true;
        bool updateWhenNotTop = false;
        // only polled for the top scene
        unsigned numScenesToPop = 0;
        std::unique_ptr<Scene> sceneToPush;
    } properties_;

    const Frame& frame_;
};

} // namespace hppv
