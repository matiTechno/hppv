#include <GLFW/glfw3.h>

#include <hppv/Renderer.hpp>

#include "Snake.hpp"
#include "Menu.hpp"
#include "Background.hpp"

Snake::Snake()
{
    properties_.opaque = false;
    properties_.maximize = true;
    nodes_.emplace_back();
}

void Snake::processInput(const bool hasInput)
{
    if(!hasInput)
        return;

    for(const auto& event: frame_.events)
    {
        if(event.type == hppv::Event::Key && event.key.action == GLFW_PRESS)
        {
            switch(event.key.key)
            {
            case GLFW_KEY_ESCAPE:
                properties_.sceneToPush = std::make_unique<Menu>(nodes_.size() - 1, Game::Paused);
            }
        }
    }
}

void Snake::update()
{}

void Snake::render(hppv::Renderer& renderer)
{
    renderer.projection(hppv::expandToMatchAspectRatio(space_, frame_.framebufferSize));

    hppv::Sprite sprite(space_);

    auto& texture = Background::resources->texSnakeFood;

    sprite.texRect = {0, 0, texture.getSize()};

    renderer.texture(texture);
    renderer.shader(hppv::Render::Tex);

    renderer.cache(sprite);
}
