#include <glm/trigonometric.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/glad.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Framebuffer.hpp>

const char* const lightSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;
in vec2 vPos;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPos - center);
    float alpha = 1 - smoothstep(0, radius, distanceFromCenter);
    vec4 sample = texture(sampler, vTexCoord);
    color = sample * vColor * alpha;
}
)";

const char* const gradientSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPos;

out vec4 color;

void main()
{
    color = vec4(0, 0, 1 - abs(vPos.y * 2 - 1), 1);
}
)";

class Lights: public hppv::PrototypeScene
{
public:
    Lights():
        hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false),
        shaderLight_({hppv::Renderer::vInstancesSource, lightSource}, "light"),
        shaderGradient_({hppv::Renderer::vInstancesSource, gradientSource}, "gradient"),
        texture_("res/mandrill.png"),
        framebuffer_(GL_RGBA8, 1)
    {
        properties_.maximize = false;
        properties_.pos = {100.f, 100.f};
        properties_.size = {600.f, 600.f};
    }

private:
    hppv::Shader shaderLight_, shaderGradient_;
    hppv::Texture texture_;
    hppv::Framebuffer framebuffer_;
    float time_ = 0.f;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.time;

        // scene background
        {
            hppv::Sprite sprite(space_.projected);

            renderer.shader(shaderGradient_);
            renderer.cache(sprite);
            renderer.flush();
        }

        // render all objects to framebuffer
        {
            framebuffer_.bind();
            framebuffer_.setSize(properties_.size);
            framebuffer_.clear();
            renderer.viewport(framebuffer_);

            {
                hppv::Sprite sprite(space_.initial);
                sprite.texRect = {0.f, 0.f, texture_.getSize()};

                renderer.shader(hppv::Render::Tex);
                renderer.texture(texture_);
                renderer.cache(sprite);

                sprite.size = {30.f, 30.f};
                sprite.color = {0.5f, 0.f, 0.5f, 0.5f};

                renderer.shader(hppv::Render::CircleColor);
                renderer.cache(sprite);
            }

            renderer.flush();
            framebuffer_.unbind();
            renderer.viewport(this);
        }

        renderer.texture(framebuffer_.getTexture());

        // ambient light
        {
            hppv::Sprite sprite;
            sprite.pos = space_.projected.pos;
            sprite.size = space_.projected.size;
            sprite.color = {0.1f, 0.1f, 0.1f, 1.f};
            sprite.texRect = {0.f, 0.f, framebuffer_.getTexture().getSize()};

            renderer.shader(hppv::Render::Tex);
            renderer.cache(sprite);
        }

        // spot lights
        renderer.shader(shaderLight_);
        {
            hppv::Circle circle;
            circle.center.x = 50.f + glm::sin(time_ / 2.f) * 20.f;
            circle.center.y = 50.f + glm::cos(time_ / 2.f) * 20.f;
            circle.radius = 25.f;
            circle.color = {1.f, 1.f, 1.f, 1.f};
            circle.texRect = hppv::mapToFramebuffer(circle.toVec4(), space_.projected, framebuffer_);

            renderer.cache(circle);

            circle.center.x = 30.f + glm::sin(time_ / 4.f) * 20.f;
            circle.center.y = 30.f + glm::cos(time_ / 4.f) * 20.f;
            circle.radius = 5.f;
            circle.color = {1.f, 1.f, 0.5f, 1.f};
            circle.texRect = hppv::mapToFramebuffer(circle.toVec4(), space_.projected, framebuffer_);

            renderer.cache(circle);
        }
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<Lights>();
    app.run();
    return 0;
}
