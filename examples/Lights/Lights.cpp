#include <glm/trigonometric.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/glad.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Framebuffer.hpp>

static const char* lightSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
   float distanceFromCenter = length(vPosition - center);
   float delta = fwidth(distanceFromCenter) * 2;
   float alpha = 1 - smoothstep(0, radius, distanceFromCenter);
   vec4 sample = texture(sampler, vTexCoords);
   color = vec4(sample.rgb * sample.a, sample.a) * vColor * alpha;
}
)";

class Lights: public hppv::PrototypeScene
{
public:
    Lights():
        hppv::PrototypeScene(hppv::Space(0.f, 0.f, 100.f, 100.f), 1.1f, false),
        shader_({hppv::Renderer::vertexSource, lightSource}, "light"),
        texture_("res/mandrill.png"),
        framebuffer_(GL_RGBA8, 1)
    {
        properties_.maximize = false;
        properties_.pos = {100.f, 100.f};
        properties_.size = {600.f, 600.f};
    }

private:
    hppv::Shader shader_;
    hppv::Texture texture_;
    hppv::Framebuffer framebuffer_;
    float time_ = 0.f;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.frameTime;

        // render all objects to framebuffer
        {
            framebuffer_.bind();
            framebuffer_.setSize(properties_.size);
            framebuffer_.clear();
            renderer.viewport(framebuffer_);

            {
                hppv::Sprite sprite;
                sprite.pos = prototype_.initialSpace.pos;
                sprite.size = prototype_.initialSpace.size;
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

        auto projection = hppv::expandToMatchAspectRatio(prototype_.space, properties_.size);

        renderer.texture(framebuffer_.getTexture(0));

        // ambient light
        {
            hppv::Sprite sprite;
            sprite.pos = projection.pos;
            sprite.size = projection.size;
            sprite.color = {0.1f, 0.1f, 0.1f, 1.f};
            sprite.texRect = {0.f, 0.f, framebuffer_.getTexture(0).getSize()};

            renderer.shader(hppv::Render::Tex);
            renderer.cache(sprite);
        }

        // spot lights
        renderer.shader(shader_);
        {
            hppv::Circle circle;
            circle.center.x = 50.f + glm::sin(time_ / 2.f) * 20.f;
            circle.center.y = 50.f + glm::cos(time_ / 2.f) * 20.f;
            circle.radius = 25.f;
            circle.color = {1.f, 1.f, 1.f, 1.f};
            circle.texRect = hppv::mapToScene({circle.center - circle.radius, glm::vec2(circle.radius * 2.f)},
                                              projection, this);

            renderer.cache(circle);

            circle.center.x = 30.f + glm::sin(time_ / 4.f) * 20.f;
            circle.center.y = 30.f + glm::cos(time_ / 3.f) * 20.f;
            circle.radius = 5.f;
            circle.color = {1.f, 1.f, 0.5f, 1.f};
            circle.texRect = hppv::mapToScene({circle.center - circle.radius, glm::vec2(circle.radius * 2.f)},
                                              projection, this);

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
