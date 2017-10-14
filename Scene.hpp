#pragma once

#include <glm/vec2.hpp>
struct Frame;

class Renderer;

class Scene
{
public:
    Scene();

    virtual ~Scene() = default;

    virtual void processInput(bool hasInput)
    {
        (void)hasInput;
    }

    virtual void render(Renderer& renderer)
    {
        (void)renderer;
    }

    struct Properties
    {
        glm::ivec2 pos;
        glm::ivec2 size;
        bool maximized;
    } properties_;

    const Frame& frame_;
};
