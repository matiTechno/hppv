#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Font.hpp>

class TestScene: public hppv::PrototypeScene
{
public:
    TestScene():
        PrototypeScene(hppv::Space(0.f, 0.f, 180.f, 100.f), 1.1f, false),
        sdfFont_("res/sdf.fnt"),
        proggy_("res/proggy.fnt")
    {
        gnu_.tex = hppv::Texture("res/gnu.png");
    }

private:
    struct
    {
        glm::vec2 pos = {5.f, 70.f};
        glm::vec2 size = {25.f, 25.f};
        hppv::Texture tex;
    }
    gnu_;

    hppv::Font sdfFont_, proggy_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        // gnu
        {
            hppv::Sprite sprite;
            sprite.pos = gnu_.pos;
            sprite.size = gnu_.size;
            sprite.texRect = {0.f, 0.f, gnu_.tex.getSize()};

            renderer.shader(hppv::Render::Tex);
            renderer.texture(gnu_.tex);
            renderer.cache(sprite);
        }
        // sdf font
        {
            // todo: serialize
            struct
            {
                char text[256] = {"The slow red squirrel jumps over the hungry bear."};
                glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
                float rotation = 0.f;
                bool sprite = true;
                glm::vec4 spriteColor = {0.1f, 0.05f, 0.05f, 0.5f};
                int comboIndex = 1;
                const char* shaders[5] = {"texture", "default", "outline", "glow", "shadow"};

                struct
                {
                    glm::vec4 color = {1.f, 0.f, 0.f, 1.f};
                    float width = 0.25f;
                }
                outline;

                struct
                {
                    glm::vec4 color = {1.f, 0.f, 0.f, 1.f};
                    float width = 0.5f;
                    bool animate = true;
                    float time = 0.f;

                }
                glow;

                struct
                {
                    glm::vec4 color = {1.f, 0.f, 0.f, 1.f};
                    float smoothing = 0.2f;
                    glm::vec2 offset = {-0.003, -0.006};
                    bool animate = true;
                    float time = 0.f;
                }
                shadow;
            }
            static sdf;

            // imgui
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.f, 0.f, 0.f, 0.9f});
                ImGui::Begin("sdf");
                ImGui::InputTextMultiline("", sdf.text, sizeof(sdf.text) / sizeof(char));
                ImGui::ColorEdit4("font color", &sdf.color.x);
                ImGui::SliderFloat("rotation", &sdf.rotation, 0.f, 2 * glm::pi<float>());
                ImGui::Checkbox("sprite", &sdf.sprite);
                ImGui::ColorEdit4("sprite color", &sdf.spriteColor.x);
                ImGui::Combo("shader", &sdf.comboIndex, sdf.shaders, sizeof(sdf.shaders) / sizeof(char*));
            }

            hppv::Text text(sdfFont_);
            text.text = sdf.text;
            text.scale = 0.4f;
            text.pos = prototype_.initialSpace.pos + prototype_.initialSpace.size / 2.f;
            text.pos -= text.getSize() / 2.f;
            text.color = sdf.color;
            text.rotation = sdf.rotation;

            if(sdf.sprite)
            {
                hppv::Sprite sprite(text);
                sprite.color = sdf.spriteColor;

                renderer.shader(hppv::Render::Color);
                renderer.cache(sprite);
            }

            if(sdf.comboIndex == 0)
            {
                renderer.shader(hppv::Render::Tex);
            }
            // imgui + uniforms
            else
            {
                auto shader = hppv::Render(int(hppv::Render::Sdf) + sdf.comboIndex - 1);
                renderer.shader(shader);

                if(shader == hppv::Render::SdfOutline)
                {
                    ImGui::ColorEdit4("outline color", &sdf.outline.color.x);
                    ImGui::SliderFloat("outline width", &sdf.outline.width, 0.f, 0.5f);

                    renderer.uniform("outlineColor", [](GLint loc){glUniform4fv(loc, 1, &sdf.outline.color.x);});
                    renderer.uniform("outlineWidth", [](GLint loc){glUniform1f(loc, sdf.outline.width);});
                }
                else if(shader == hppv::Render::SdfGlow)
                {
                    if(sdf.glow.animate)
                    {
                        sdf.glow.time += frame_.frameTime;
                        sdf.glow.width = (glm::sin(sdf.glow.time * 2.f * glm::pi<float>() / 4.f) + 1.f) / 4.f;
                    }

                    ImGui::ColorEdit4("glow color", &sdf.glow.color.x);
                    ImGui::SliderFloat("glow width", &sdf.glow.width, 0.f, 0.5f);
                    ImGui::Checkbox("animate", &sdf.glow.animate);

                    renderer.uniform("glowColor", [](GLint loc){glUniform4fv(loc, 1, &sdf.glow.color.x);});
                    renderer.uniform("glowWidth", [](GLint loc){glUniform1f(loc, sdf.glow.width);});
                }
                else if(shader == hppv::Render::SdfShadow)
                {
                    if(sdf.shadow.animate)
                    {
                        sdf.shadow.time += frame_.frameTime;
                        sdf.shadow.offset.x = glm::sin(sdf.shadow.time * 2.f * glm::pi<float>() / 8.f) / 200.f;
                        sdf.shadow.offset.y = glm::cos(sdf.shadow.time * 2.f * glm::pi<float>() / 8.f) / 200.f;
                    }

                    ImGui::ColorEdit4("shadow color", &sdf.shadow.color.x);
                    ImGui::SliderFloat("shadow smoothing", &sdf.shadow.smoothing, 0.f, 0.5f);
                    ImGui::SliderFloat2("shadow offset", &sdf.shadow.offset.x, -0.01f, 0.01f);
                    ImGui::Checkbox("animate", &sdf.shadow.animate);

                    renderer.uniform("shadowColor", [](GLint loc){glUniform4fv(loc, 1, &sdf.shadow.color.x);});
                    renderer.uniform("shadowSmoothing", [](GLint loc){glUniform1f(loc, sdf.shadow.smoothing);});
                    renderer.uniform("shadowOffset", [](GLint loc){glUniform2fv(loc, 1, &sdf.shadow.offset.x);});
                }
            }
            // imgui end
            {
                ImGui::End();
                ImGui::PopStyleColor();
            }

            renderer.texture(sdfFont_.getTexture());
            renderer.cache(text);
        }

        renderer.projection(this->properties_);

        // some text at the top
        {
            hppv::Text text(proggy_);
            text.pos = properties_.pos + glm::ivec2{10, 10};
            text.color = {0.f, 0.8f, 0.4f, 1.f};
            text.text = "The quick brown fox jumps over the lazy dog. #include <iostream> int main(){\n"
                        "std::cout << \"Hello World!\" << std::endl; return 0;}";

            renderer.shader(hppv::Render::Tex);
            renderer.texture(proggy_.getTexture());
            renderer.cache(text);
        }
        // gnu info
        {
            hppv::Text text(proggy_);
            text.text = "This is a small gnu creature!";
            {
                auto projection = hppv::expandToMatchAspectRatio(prototype_.space, frame_.framebufferSize);
                auto rect = hppv::iMapToScene({gnu_.pos, gnu_.size}, projection, this);
                glm::ivec2 size = text.getSize();
                text.pos = {rect.x + rect.z / 2 - size.x / 2, rect.y - size.y};
            }

            hppv::Sprite sprite(text);
            sprite.color = {0.4f, 0.1f, 0.1f, 1.f};

            renderer.shader(hppv::Render::Color);
            renderer.cache(sprite);

            renderer.shader(hppv::Render::Tex);
            renderer.texture(proggy_.getTexture());
            renderer.cache(text);
        }
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<TestScene>();
    app.run();
    return 0;
}
