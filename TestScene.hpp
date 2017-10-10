#pragma once

#include "Scene.hpp"
#include "Renderer.hpp"

class TestScene: public Scene
{
public:
    TestScene()
    {
        camera_.pos = {200.f, 200.f};
        camera_.size = {1000.f, 1000.f};
    }

    void render(Renderer& renderer, Rect projection) override
    {
        glm::vec2 offset(20.f, 20.f);

        Sprite sprite;
        sprite.pos = camera_.pos + offset;
        sprite.size = camera_.size - 2.f * offset;
        sprite.color = {1.f, 0.f, 0.f, 0.5f};
        //sprite.rotation = 3.14f / 4.f;

        renderer.cache(sprite);

        sprite.pos = projection.pos + offset;
        sprite.size = projection.size - 2.f * offset;
        sprite.color = {0.f, 1.f, 0.f, 0.1f};
        sprite.rotation = 0.f;

        renderer.cache(sprite);
    }
};
