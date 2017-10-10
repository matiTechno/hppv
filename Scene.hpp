#pragma once

#include "Rect.hpp"

class Renderer;

class Scene
{
public:
    virtual ~Scene() = default;

    virtual void render(Renderer& renderer, Rect projection)
    {
        (void)renderer;
        (void)projection;
    }

    Rect camera_;
};
