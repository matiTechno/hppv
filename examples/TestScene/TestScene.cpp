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
        PrototypeScene({0.f, 0.f, 180.f, 100.f}, 1.1f, false),
        sdfFont_("res/sdf.fnt"),
        proggy_("res/proggy.fnt")
    {
        gnu_.tex = hppv::Texture("res/gnu.png");
    }

private:
    hppv::Font sdfFont_, proggy_;

    struct
    {
        glm::vec2 pos = {5.f, 70.f};
        glm::vec2 size = {25.f, 25.f};
        hppv::Texture tex;
    }
    gnu_;

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
    sdf_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.antialiasedSprites(true);

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
            ImGui::Begin("sdf");
            ImGui::InputTextMultiline("", sdf_.text, sizeof(sdf_.text) / sizeof(char));
            ImGui::ColorEdit4("font color", &sdf_.color.x);
            ImGui::SliderFloat("rotation", &sdf_.rotation, 0.f, 2 * glm::pi<float>());
            ImGui::Checkbox("sprite", &sdf_.sprite);
            ImGui::ColorEdit4("sprite color", &sdf_.spriteColor.x);
            ImGui::Combo("shader", &sdf_.comboIndex, sdf_.shaders, sizeof(sdf_.shaders) / sizeof(char*));

            hppv::Text text(sdfFont_);
            text.text = sdf_.text;
            text.scale = 0.4f;
            text.pos = space_.initial.pos + space_.initial.size / 2.f;
            text.pos -= text.getSize() / 2.f;
            text.color = sdf_.color;
            text.rotation = sdf_.rotation;

            if(sdf_.sprite)
            {
                hppv::Sprite sprite(text);
                sprite.color = sdf_.spriteColor;

                renderer.shader(hppv::Render::Color);
                renderer.cache(sprite);
            }

            if(sdf_.comboIndex == 0)
            {
                renderer.shader(hppv::Render::Tex);
            }
            else
            {
                const auto renderMode = hppv::Render(static_cast<int>(hppv::Render::Sdf) + sdf_.comboIndex - 1);
                renderer.shader(renderMode);

                if(renderMode == hppv::Render::SdfOutline)
                {
                    ImGui::ColorEdit4("outline color", &sdf_.outline.color.x);
                    ImGui::SliderFloat("outline width", &sdf_.outline.width, 0.f, 0.5f);

                    renderer.uniform4f("outlineColor", sdf_.outline.color);
                    renderer.uniform1f("outlineWidth", sdf_.outline.width);
                }
                else if(renderMode == hppv::Render::SdfGlow)
                {
                    if(sdf_.glow.animate)
                    {
                        sdf_.glow.time += frame_.time;
                        sdf_.glow.width = (glm::sin(sdf_.glow.time * 2.f * glm::pi<float>() / 4.f) + 1.f) / 4.f;
                    }

                    ImGui::ColorEdit4("glow color", &sdf_.glow.color.x);
                    ImGui::SliderFloat("glow width", &sdf_.glow.width, 0.f, 0.5f);
                    ImGui::Checkbox("animate", &sdf_.glow.animate);

                    renderer.uniform4f("glowColor", sdf_.glow.color);
                    renderer.uniform1f("glowWidth", sdf_.glow.width);
                }
                else if(renderMode == hppv::Render::SdfShadow)
                {
                    if(sdf_.shadow.animate)
                    {
                        sdf_.shadow.time += frame_.time;
                        sdf_.shadow.offset.x = glm::sin(sdf_.shadow.time * 2.f * glm::pi<float>() / 8.f) / 200.f;
                        sdf_.shadow.offset.y = glm::cos(sdf_.shadow.time * 2.f * glm::pi<float>() / 8.f) / 200.f;
                    }

                    ImGui::ColorEdit4("shadow color", &sdf_.shadow.color.x);
                    ImGui::SliderFloat("shadow smoothing", &sdf_.shadow.smoothing, 0.f, 0.5f);
                    ImGui::SliderFloat2("shadow offset", &sdf_.shadow.offset.x, -0.01f, 0.01f);
                    ImGui::Checkbox("animate", &sdf_.shadow.animate);

                    renderer.uniform4f("shadowColor", sdf_.shadow.color);
                    renderer.uniform1f("shadowSmoothing", sdf_.shadow.smoothing);
                    renderer.uniform2f("shadowOffset", sdf_.shadow.offset);
                }
            }
            ImGui::End();

            renderer.texture(sdfFont_.getTexture());
            renderer.cache(text);
        }

        renderer.projection({0, 0, properties_.size});

        // some text at the top
        {
            hppv::Text text(proggy_);
            text.pos = {10, 10};
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
                const auto rect = hppv::iMapToScene({gnu_.pos, gnu_.size}, space_.projected, this);
                const glm::ivec2 size = text.getSize();
                text.pos = {rect.x + (rect.z - size.x) / 2, rect.y - size.y};
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
