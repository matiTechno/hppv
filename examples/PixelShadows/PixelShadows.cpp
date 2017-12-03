// github.com/mattdesl/lwjgl-basics/wiki/2D-Pixel-Perfect-Shadows

#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/glad.h>

static const char* shadowSource = R"(

#fragment
#version 330

in vec2 vTexCoord;

uniform sampler2D sampler;

const float Pi = 3.14;
const float threshold = 0.75;

out vec4 color;

void main()
{
    float distance = 1;

    int texSizeY = textureSize(sampler, 0).y;

    for(float y = 0; y < texSizeY; y += 1)
    {
        vec2 norm = vec2(vTexCoord.x, y / texSizeY) * 2 - 1;
        float theta = Pi * 1.5 + norm.x * Pi;
        float r = (1 + norm.y) * 0.5;
        vec2 texCoord = vec2(-r * sin(theta), -r * cos(theta)) / 2 + 0.5;
        vec4 sample = texture(sampler, texCoord);
        float dst = y / texSizeY;
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
in vec2 vTexCoord;

uniform sampler2D sampler;

const float Pi = 3.14;

out vec4 color;

float sample(vec2 texCoord, float r)
{
    return step(r, texture(sampler, texCoord).r);
}

void main()
{
    vec2 norm = vTexCoord * 2 - 1;
    float theta = atan(norm.y, norm.x);
    float r = length(norm);
    float coordX = (theta + Pi) / (2 * Pi);
    vec2 tc = vec2(coordX, 0);
    float center = sample(tc, r);
    float blur = (1.0 / textureSize(sampler, 0).x) * smoothstep(0, 1, r);
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
        texTile_("tile.png"),
        shaderShadow_({hppv::Renderer::vInstancesSource, shadowSource}, "shadow"),
        shaderLight_({hppv::Renderer::vInstancesSource, lightSource}, "light")
    {
        {
            hppv::Circle circle;
            circle.center = {50.f, 50.f};
            circle.radius = 35.f;
            circle.color = {1.f, 0.4f, 0.2f, 0.f};
            lights_.push_back(circle);
        }
        {
            hppv::Circle circle;
            circle.center = {0.f, 0.f};
            circle.radius = 50.f;
            circle.color = {0.6f, 0.6f, 1.f, 0.f};
            lights_.push_back(circle);
        }
        {
            hppv::Circle circle;
            circle.radius = 55.f;
            circle.color = {0.f, 1.f, 0.f, 0.f};
            lights_.push_back(circle);
        }

        hppv::Sprite sprite;
        sprite.color = {0.6f, 0.3f, 0.3f, 1.f};
        sprite.texRect = {0, 0, texTile_.getSize()};

        {
            sprite.pos = {10.f, 10.f};
            sprite.size = {30.f, 8.f};
            sprite.rotation = glm::pi<float>() / 8.f;
            objects_.push_back(sprite);
        }
        {
            sprite.pos = {60.f, 60.f};
            sprite.size = {40.f, 40.f};
            sprite.rotation = -0.1f;
            objects_.push_back(sprite);
        }
        {
            sprite.pos = {40.f, 30.f};
            sprite.size = {5.f, 5.f};
            sprite.rotation = 0.f;
            objects_.push_back(sprite);
        }
    }

private:
    enum {LightRays = 256};

    hppv::Framebuffer fbOcclusion_, fbShadow_;
    hppv::Texture texTile_;
    hppv::Shader shaderShadow_, shaderLight_;
    std::vector<hppv::Circle> lights_;
    std::vector<hppv::Sprite> objects_;
    float time_ = 0.f;

    void update() override
    {
        time_ += frame_.time;

        lights_[2].center.x = 90.f + 40 * glm::sin(time_);
        lights_[2].center.y = 80.f + 40 * glm::cos(time_);
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.antialiasedSprites(true);

        for(auto& light: lights_)
        {
            // occlusion map
            {
                fbOcclusion_.bind();
                fbOcclusion_.setSize(glm::ivec2(LightRays));
                fbOcclusion_.clear();
                renderer.viewport(fbOcclusion_);
                renderer.projection(light.toSpace());
                {
                    renderer.shader(hppv::Render::Color);

                    for(auto& object: objects_)
                    {
                        renderer.cache(object);
                    }
                }
                renderer.projection(space_.projected);
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
                    renderer.texture(occlusionTex);
                    renderer.cache(sprite);
                }
                renderer.flush();
                renderer.viewport(this);
                fbShadow_.unbind();
            }
            // light
            {
                hppv::Sprite sprite(light);
                sprite.color = light.color;
                auto& shadowTex = fbShadow_.getTexture();
                sprite.texRect = {0, 0, shadowTex.getSize()};

                renderer.flipTextureY(true); // why?
                renderer.shader(shaderLight_);
                renderer.texture(shadowTex);
                renderer.cache(sprite);
                renderer.flipTextureY(false);
                renderer.flush();
            }
        }

        // objects
        {
            renderer.shader(hppv::Render::Tex);
            renderer.texture(texTile_);

            for(auto& object: objects_)
            {
                renderer.cache(object);
            }
        }
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<PixelShadows>();
    app.run();
    return 0;
}
