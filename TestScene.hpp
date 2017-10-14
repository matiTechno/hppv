#pragma once

#include "Scene.hpp"
#include "Renderer.hpp"
#include "Space.hpp"

class TestScene: public Scene
{
public:
    TestScene()
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
        properties_.maximized = false;
    }

    void render(Renderer& renderer) override
    {
        Space space(0.f, 0.f, 100.f, 100.f);
        auto projectedSpace = createExpandedToMatchAspectRatio(space, properties_.size);
        renderer.setProjection(projectedSpace);
        {
            Sprite sprite;
            sprite.color = {0.f, 1.f, 0.f, 0.1f};
            sprite.pos = space.pos;
            sprite.size = space.size;
            renderer.cache(sprite);
        }
        {
            Sprite sprite;
            sprite.color = {1.f, 0.f, 0.f, 0.8f};
            sprite.pos = {25.f, 25.f};
            sprite.size = {50.f, 50.f};
            renderer.cache(sprite);
        }
        renderer.flush();
    }
};
