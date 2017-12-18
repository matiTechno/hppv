#include <cassert>

#include <GLFW/glfw3.h>

#include <hppv/Renderer.hpp>

#include "Background.hpp"
#include "Snake.hpp"

const char* const waveSource = R"(

#fragment
#version 330

in vec2 vPos;

uniform float time;
uniform vec2 projSize;

const float vel = 15;
const float len = 400;
const float Pi = 3.14;

out vec4 color;

void main()
{
    float f = vel / len;
    vec2 pos = vPos * projSize;
    float r = length(pos - projSize / 2);
    float h = sin(2 * Pi / len * r - 2 * Pi * f * time) / 2 + 0.5;
    color = vec4(0, h, 0, 0);
}
)";

Resources* Background::resources = nullptr;

Background::Background():
    shWave_({hppv::Renderer::vInstancesSource, waveSource}, "shWave_")
{
    assert(resources == nullptr);
    resources = &resources_;

    resources_.font = hppv::Font("CPMono_v07 Bold.otf", 64);
    resources_.texSnakeFood = hppv::Texture("food.png");

    {
        std::random_device rd;
        resources_.rng.seed(rd());
    }

    properties_.maximize = true;
    properties_.updateWhenNotTop = true;
}

void Background::processInput(const bool hasInput)
{
    if(hasInput == false)
        return;

    for(const auto& event: frame_.events)
    {
        if(event.type == hppv::Event::Key && event.key.key == GLFW_KEY_ENTER
                                          && event.key.action == GLFW_PRESS)
        {
            properties_.sceneToPush = std::make_unique<Snake>();
        }
    }
}

void Background::update()
{
    time_ += frame_.time;

    animation_.accumulator += frame_.time;

    while(animation_.accumulator >= animation_.timestep)
    {
        animation_.accumulator -= animation_.timestep;
        ++animation_.count;

        if(animation_.count > 3)
        {
            animation_.count = 0;
        }
    }

    shWave_.bind();
    shWave_.uniform1f("time", time_);
    shWave_.uniform2f("projSize", properties_.size);
}

void Background::render(hppv::Renderer& renderer)
{
    renderer.projection({0, 0, properties_.size});

    {
        renderer.shader(shWave_);

        hppv::Sprite sprite;
        sprite.pos = {0.f, 0.f};
        sprite.size = properties_.size;

        renderer.cache(sprite);
    }

    hppv::Text text(resources_.font);
    text.color = {0.f, 0.f, 0.f, 1.f};
    text.text = "press ENTER to play ...";
    text.pos = (properties_.size - glm::ivec2(text.getSize())) / 2;

    hppv::Sprite sprite(text);
    sprite.color = {1.f, 0.5f, 0.5f, 0.f};

    renderer.shader(hppv::Render::Color);

    renderer.cache(sprite);

    text.text.erase(text.text.end() - 3, text.text.end());

    for(auto i = 0; i < animation_.count; ++i)
    {
        text.text += '.';
    }

    renderer.texture(resources_.font.getTexture());
    renderer.shader(hppv::Render::Font);

    {
        auto text2 = text;
        text2.color = {0.f, 1.f, 0.f, 1.f};
        text2.pos += 3.f;

        renderer.cache(text2);
    }

    renderer.cache(text);
}
