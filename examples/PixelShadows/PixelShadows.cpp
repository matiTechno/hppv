#include <glm/gtc/constants.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>
#include <hppv/Shader.hpp>

static const char* shadowSource = R"(

#fragment
#version 330

in vec2 vTexCoords;

uniform sampler2D sampler;
uniform vec2 resolution;

const float Pi = 3.14;
const float threshold = 0.75;

out vec4 color;

void main()
{
    float distance = 1;

    for(float y = 0; y < resolution.y; y += 1)
    {
        vec2 norm = vec2(vTexCoords.x, y / resolution.y) * 2 - 1;
        float theta = Pi * 1.5 + norm.x * Pi;
        float r = (1 + norm.y) * 0.5;

        vec2 texCoords = vec2(-r * sin(theta), -r * cos(theta)) / 2 + 0.5;

        vec4 sample = texture2D(sampler, texCoords);

        float dst = y / resolution.y;

        float caster = sample.a;

        if(caster > threshold)
        {
            distance = min(distance, dst);
        }
    }

    color = vec4(vec3(distance), 1);
}
)";

static const char* lightSource = R"(

#fragment
#version 330

void main()
{}
)";

class PixelShadows: public hppv::PrototypeScene
{
public:
    PixelShadows():
        hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false),
        fbOcclusion_(GL_RGBA8, 1),
        fbShadow_(GL_RGBA8, 1),
        shaderShadow_({hppv::Renderer::vertexSource, shadowSource}, "shadow"),
        shaderLight_({hppv::Renderer::vertexSource, lightSource}, "light")
    {}

private:
    enum {LightSize = 256};

    hppv::Framebuffer fbOcclusion_, fbShadow_;
    hppv::Shader shaderShadow_, shaderLight_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        using hppv::Sprite;

        renderer.shader(hppv::Render::Color);
        renderer.antialiasedSprites(true);

        /*
        {
            Sprite sprite(prototype_.space);
            sprite.color = {0.9f, 0.5f, 0.3f, 1.f};
            renderer.cache(sprite);
        }
        */

        // occlusion map
        fbOcclusion_.bind();
        fbOcclusion_.setSize(glm::ivec2(LightSize));
        fbOcclusion_.clear();
        renderer.viewport(fbOcclusion_);
        {
            Sprite sprite;
            sprite.pos = {10.f, 10.f};
            sprite.size = {30.f, 8.f};
            sprite.rotation = glm::pi<float>() / 8.f;

            renderer.cache(sprite);

            sprite.pos = {50.f, 50.f};
            sprite.size = {40.f, 40.f};
            sprite.rotation = -0.1f;

            renderer.cache(sprite);
        }
        renderer.flush();

        // shadow map
        fbShadow_.bind();
        fbShadow_.setSize({LightSize, 1});
        fbShadow_.clear();
        renderer.viewport(fbShadow_);
        {
            Sprite sprite(prototype_.space);
            auto& occlusionTex = fbOcclusion_.getTexture();
            sprite.texRect = {0.f, 0.f, occlusionTex.getSize()};

            renderer.uniform("resolution", occlusionTex.getSize());
            renderer.texture(occlusionTex);
            renderer.shader(shaderShadow_);
        }
        renderer.flush();
        renderer.viewport(this);
        fbShadow_.unbind();
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return -1;
    app.pushScene<PixelShadows>();
    app.run();
    return 0;
}
