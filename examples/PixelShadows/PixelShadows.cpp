#include <vector>

#include <glm/gtc/constants.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/glad.h>

static const char* shadowSource = R"(

#fragment
#version 330

in vec2 vTexCoords;

uniform sampler2D sampler;
uniform float resolution;

const float Pi = 3.14;
const float threshold = 0.75;

out vec4 color;

void main()
{
    float distance = 1;

    for(float y = 0; y < resolution; y += 1)
    {
        vec2 norm = vec2(vTexCoords.x, y / resolution) * 2 - 1;
        float theta = Pi * 1.5 + norm.x * Pi;
        float r = (1 + norm.y) * 0.5;
        vec2 texCoords = vec2(-r * sin(theta), -r * cos(theta)) / 2 + 0.5;
        vec4 sample = texture(sampler, texCoords);
        float dst = y / resolution;
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

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;
uniform float resolution;

const float Pi = 3.14;

out vec4 color;

float sample(vec2 texCoords, float r)
{
    return step(r, texture(sampler, texCoords).r);
}

void main()
{

    vec2 norm = vTexCoords * 2 - 1;
    float theta = atan(norm.y, norm.x);
    float r = length(norm);
    float coordX = (theta + Pi) / (2 * Pi);
    vec2 tc = vec2(coordX, 0);
    float center = sample(tc, r);
    float blur = (1 / resolution) * smoothstep(0, 1, r);
    float sum = 0;
    sum += sample(vec2(tc.x - 4 * blur, tc.y), r) * 0.05;
    sum += sample(vec2(tc.x - 3 * blur, tc.y), r) * 0.09;
    sum += sample(vec2(tc.x - 2 * blur, tc.y), r) * 0.12;
    sum += sample(vec2(tc.x - 1 * blur, tc.y), r) * 0.15;
    sum += center * 0.16;
    sum += sample(vec2(tc.x + 1 * blur, tc.y), r) * 0.15;
    sum += sample(vec2(tc.x + 2 * blur, tc.y), r) * 0.12;
    sum += sample(vec2(tc.x + 3 * blur, tc.y), r) * 0.09;
    sum += sample(vec2(tc.x + 4 * blur, tc.y), r) * 0.05;

    color = vColor * vec4(sum * smoothstep(1, 0, r));
}
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
    {
        {
            light_.color = {1.f, 0.5f, 0.2f, 0.f};
            light_.radius = 2.f;
        }
        {
            hppv::Sprite sprite;
            sprite.pos = {10.f, 10.f};
            sprite.size = {30.f, 8.f};
            sprite.color = {0.2f, 0.f, 0.f, 1.f};
            sprite.rotation = glm::pi<float>() / 8.f;
            sprites_.push_back(sprite);
        }
        {
            hppv::Sprite sprite;
            sprite.pos = {60.f, 60.f};
            sprite.size = {40.f, 40.f};
            sprite.color = {0.f, 0.3f, 0.f, 1.f};
            sprite.rotation = -0.1f;
            sprites_.push_back(sprite);
        }
        {
            hppv::Sprite sprite;
            sprite.pos = {40.f, 30.f};
            sprite.size = {5.f, 5.f};
            sprite.color = {0.2f, 0.2f, 0.2f, 1.f};
            sprites_.push_back(sprite);
        }
    }

private:
    enum {LightRays = 256};

    hppv::Framebuffer fbOcclusion_, fbShadow_;
    hppv::Shader shaderShadow_, shaderLight_;
    hppv::Circle light_;
    std::vector<hppv::Sprite> sprites_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.antialiasedSprites(true);
        light_.center = space_.current.pos + space_.current.size / 2.f;

        // occlusion map
        {
            fbOcclusion_.bind();
            fbOcclusion_.setSize(glm::ivec2(LightRays));
            fbOcclusion_.clear();
            renderer.viewport(fbOcclusion_);
            {
                renderer.shader(hppv::Render::Color);

                for(auto& sprite: sprites_)
                {
                    renderer.cache(sprite);
                }
            }
            renderer.flush();
        }
        // shadow map
        {
            fbShadow_.bind();
            fbShadow_.setSize({LightRays, 1});
            fbShadow_.clear();
            renderer.viewport(fbShadow_);
            {
                hppv::Sprite sprite(space_.projected);
                auto& occlusionTex = fbOcclusion_.getTexture();
                sprite.texRect = {0, 0, occlusionTex.getSize()};

                renderer.shader(shaderShadow_);
                renderer.uniform1f("resolution", LightRays);
                renderer.texture(occlusionTex);
                renderer.cache(sprite);
            }
            renderer.flush();
            renderer.viewport(this);
            fbShadow_.unbind();
        }
        // shadows
        {
            hppv::Sprite sprite(space_.projected);
            sprite.color = light_.color;
            auto& shadowTex = fbShadow_.getTexture();
            sprite.texRect = {0, 0, shadowTex.getSize()};

            renderer.flipTextureY(true);
            renderer.shader(shaderLight_);
            renderer.uniform1f("resolution", LightRays);
            renderer.texture(shadowTex);
            renderer.cache(sprite);
            renderer.flipTextureY(false);
        }
        // objects
        {
            renderer.shader(hppv::Render::Color);

            for(auto& sprite: sprites_)
            {
                renderer.cache(sprite);
            }

            renderer.shader(hppv::Render::CircleColor);
            renderer.cache(light_);
        }
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
