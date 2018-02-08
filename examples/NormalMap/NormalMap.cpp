// github.com/mattdesl/lwjgl-basics/wiki/ShaderLesson6

#include <vector>

#include <glm/vec3.hpp>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Shader.hpp>
#include <hppv/imgui.h>
#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>

#include "Glsl.hpp"
#include "../run.hpp"

struct Texture
{
    hppv::Texture diffuse;
    hppv::Texture normal;
};

// todo: gamma correction?

class NormalMap: public hppv::Prototype
{
public:
    NormalMap():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f}),
        fb_(GL_RGBA8, 2),
        shDeferred_({hppv::Renderer::vInstancesSource, deferredSource}, "shDeferred_"),
        shTexture_({hppv::Renderer::vInstancesSource, textureSource}, "shTexture_"),
        texRock_{hppv::Texture("res/rock_diffuse.png"), hppv::Texture("res/rock_normal.png")},
        texTeapot_{hppv::Texture("res/teapot_diffuse.png"), hppv::Texture("res/teapot_normal.png")}
    {
        prototype_.zoomFactor = 1.05f;

        light_.pos.z = 2.f;

        hppv::Shader* shaders[] = {&shDeferred_, &shTexture_};

        for(auto shader: shaders)
        {
            shader->bind();
            shader->uniform1i("samplerDiffuse", Diffuse);
            shader->uniform1i("samplerNormal", Normal);
        }
    }

private:
    enum {Diffuse = 0, Normal = 1};

    hppv::Framebuffer fb_;
    hppv::Shader shDeferred_, shTexture_;
    Texture texRock_, texTeapot_;

    struct
    {
        glm::vec4 pos;
        glm::vec3 color = {1.f, 1.f, 1.f};
        glm::vec3 ambientColor = {0.05f, 0.05f, 0.05f};
        glm::vec3 attenuationCoeffs = {1.f, 0.001f, 0.002f};
    }
    light_;

    void prototypeProcessInput(const hppv::PInput input) override
    {
        const auto pos = hppv::mapCursor(input.cursorPos, space_.projected, this);
        light_.pos.x = pos.x;
        light_.pos.y = pos.y;
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        {
            ImGui::SliderFloat("pos.z", &light_.pos.z, -10.f, 50.f);
            ImGui::ColorEdit3("color", &light_.color.x);
            ImGui::ColorEdit3("ambientColor", &light_.ambientColor.x);
            ImGui::InputFloat3("attenuationCoeffs", &light_.attenuationCoeffs.x);
        }
        ImGui::End();

        fb_.bind();
        fb_.setSize(properties_.size);
        fb_.clear();
        renderer.viewport(fb_);

        renderer.shader(shTexture_);

        // cache the game objects (textured sprites)
        {
            {
                renderer.texture(texRock_.diffuse, Diffuse);
                renderer.texture(texRock_.normal, Normal);
                renderer.uniform1i("invertNormalY", false);

                hppv::Sprite sprite;
                sprite.pos = space_.initial.pos;
                sprite.size = space_.initial.size;

                renderer.cache(sprite);
            }
            {
                renderer.texture(texTeapot_.diffuse, Diffuse);
                renderer.texture(texTeapot_.normal, Normal);
                renderer.uniform1i("invertNormalY", true);

                hppv::Sprite sprite;
                sprite.pos = {0.f, 0.f};
                sprite.size = {20.f, 20.f};

                renderer.cache(sprite);
            }
        }

        renderer.flush();
        renderer.viewport(this);
        fb_.unbind();

        renderer.shader(shDeferred_);

        renderer.uniform3f("lightPos", light_.pos);
        renderer.uniform3f("lightColor", light_.color);
        renderer.uniform3f("ambientColor", light_.ambientColor);
        renderer.uniform3f("attCoeffs", light_.attenuationCoeffs);
        renderer.uniform2f("projPos", space_.projected.pos);
        renderer.uniform2f("projSize", space_.projected.size);

        renderer.texture(fb_.getTexture(Diffuse), Diffuse);
        renderer.texture(fb_.getTexture(Normal), Normal);

        {
            hppv::Sprite sprite;
            sprite.pos = space_.projected.pos;
            sprite.size = space_.projected.size;

            renderer.cache(sprite);
        }
    }
};

RUN(NormalMap)
