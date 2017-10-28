#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/external/imgui.h>
#include <hppv/Texture.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Font.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/external/glad.h>

class TestScene: public hppv::PrototypeScene
{
public:
    TestScene():
        PrototypeScene(hppv::Space(0.f, 0.f, 100.f, 100.f), 1.1f, false),
        texture_("gnud.png"),
        font_("/home/mati/font.fnt")
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
        //properties_.maximize = false;

        //hppv::App::hideCursor(true);
    }

private:
    hppv::Texture texture_;
    hppv::Font font_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::ShowTestWindow();

        {
                renderer.setShader(hppv::Render::Color);
                hppv::Sprite sprite;
                sprite.color = {0.1f, 0.1f, 0.1f, 1.f};
                sprite.pos = prototype_.initialSpace.pos;
                sprite.size = prototype_.initialSpace.size;
                renderer.cache(sprite);
        }
            {
                renderer.setShader(hppv::Render::Color);
                hppv::Sprite sprite;
                sprite.color = {1.f, 0.f, 0.f, 0.f};
                sprite.pos = {0.f, 0.f};
                sprite.size = {20.f, 20.f};
                renderer.cache(sprite);
            }

            {
                renderer.setShader(hppv::Render::TexturePremultiplyAlpha);
                renderer.setTexture(font_.getTexture());
                hppv::Sprite sprite;
                //sprite.color = {1.f, 0.f, 1.f, 0.4f};
                sprite.pos = {45.f, 45.f};
                sprite.size = {30.f, 30.f};
                sprite.texRect = {0, 0, font_.getTexture().getSize()};
                renderer.cache(sprite);

                renderer.setShader(hppv::Render::Font);
                renderer.setTexture(font_.getTexture());
                hppv::Text text;
                text.text = "Hula dupal\nBarcelona !!! :D";
                text.font = &font_;
                text.pos = {0.f, 0.f};
                text.scale = 1.f;
                renderer.cache(text);
        }
        {
            renderer.setShader(hppv::Render::CircleColor);
            hppv::Circle circle;
            circle.color = {1.f, 1.f, 1.f, 0.2f};
            circle.center = {50.f, 50.f};
            circle.radius = {20.f};
            //renderer.cache(circle);
        }
        {
            renderer.setShader(hppv::Render::Texture);
            renderer.setTexture(texture_);
            hppv::Sprite sprite;
            sprite.pos = {200.f, 200.f};
            sprite.size = {100.f, 100.f};
            sprite.texRect = {0.f, 0.f, texture_.getSize()};
            //sprite.color = {0.2f, 0.2f, 0.2f, 0.5f};
            renderer.cache(sprite);
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
