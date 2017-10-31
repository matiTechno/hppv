#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/external/glad.h>
#include <hppv/Space.hpp>

#include <glm/trigonometric.hpp>

static const char* circleTextureShaderSource = R"(

#fragment
#version 330

out vec4 color;

in vec4 vColor;

in vec2 vPosition;

in vec2 vTexCoords;

uniform float radius = 0.5;
uniform vec2 center = vec2(0.5, 0.5);

uniform sampler2D sampler;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = smoothstep(radius - delta * 2,
                             radius,
                             distanceFromCenter);

    color = texture(sampler, vTexCoords) * vec4(vColor.rgb, vColor.a * (1 - alpha) * (radius - distanceFromCenter ));
}

)";

class Lights: public hppv::PrototypeScene
{
public:
    Lights():
        hppv::PrototypeScene(hppv::Space(0.f, 0.f, 100.f, 100.f), 1.1f, false),
        texture_("mandrill.png"),
        framebuffer_(GL_RGBA8, 1),
        shader_({hppv::Renderer::vertexShaderSource, circleTextureShaderSource}, "circle")
    {
    }

private:
    hppv::Texture texture_;
    hppv::Framebuffer framebuffer_;
    float time_ = 0.f;
    hppv::Shader shader_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.frameTime;

        framebuffer_.bind();
        framebuffer_.setSize(properties_.size);
        framebuffer_.clear();

        {
            renderer.setShader(hppv::Render::TextureFlippedY);
            renderer.setTexture(texture_);

            hppv::Sprite sprite;
            sprite.pos = prototype_.space.pos;
            sprite.size = prototype_.space.size;
            sprite.texRect = {0.f, 0.f, texture_.getSize()};

            renderer.cache(sprite);
        }

        renderer.flush();
        framebuffer_.unbind();

        auto projection = hppv::expandToMatchAspectRatio(prototype_.space, frame_.framebufferSize);

        {
            renderer.setShader(hppv::Render::Texture);
            renderer.setTexture(framebuffer_.getTexture(0));

            hppv::Sprite sprite;
            sprite.pos = prototype_.initialSpace.pos;
            sprite.size = prototype_.initialSpace.size;
            sprite.color.a = 0.05f;
            sprite.color.b = 0.f;
            sprite.texRect = hppv::mapToScene({sprite.pos, sprite.size}, projection, *this);
            renderer.cache(sprite);
        }

        {
           // renderer.setShader(shader_);

            hppv::Circle circle;
            circle.center.x = 300.f + glm::sin(time_ / 2.f) * 150.f;
            circle.center.y = 300.f + glm::cos(time_ / 2.f) * 150.f;
            circle.radius = 100.f;
            circle.color = {1.f, 1.f, 0.f, 1.f};
            //circle.color.a = glm::sin(time_) / 2.f + 1.f;
            circle.texRect = hppv::mapToScene({circle.center - circle.radius, glm::vec2(circle.radius * 2.f)},
                                                projection, *this);

            //renderer.cache(circle);
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
