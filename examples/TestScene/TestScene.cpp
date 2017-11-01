#include <glm/gtc/constants.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Font.hpp>

class TestScene: public hppv::PrototypeScene
{
public:
    TestScene():
        PrototypeScene(hppv::Space(0.f, 0.f, 100.f, 100.f), 1.1f, false),
        texture_("res/gnu.png"),
        font_("res/font.fnt")
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
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
            sprite.color = {0.05f, 0.f, 0.05f, 1.f};
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
            renderer.setShader(hppv::Render::TexPremultiplyAlpha);
            renderer.setTexture(font_.getTexture());
            hppv::Sprite sprite;
            sprite.pos = {45.f, 45.f};
            sprite.size = {30.f, 30.f};
            sprite.texRect = {0, 0, font_.getTexture().getSize()};
            renderer.cache(sprite);
        }
        {
            hppv::Text text;
            text.text = "Hula dupal\nBarcelona !!! :D";
            text.font = &font_;
            text.pos = {0.f, 0.f};
            text.scale = 0.5f;
            text.color = {1.f, 1.f, 0.f, 1.f};
            text.rotation = glm::pi<float>() / 4.f;
            text.rotationPoint -= text.getSize() / 2.f;

            renderer.setShader(hppv::Render::Color);

            hppv::Sprite sprite;
            sprite.size = glm::vec2(2.f);
            sprite.color = {0.f, 1.f, 0.f, 1.f};
            sprite.pos = text.pos + text.getSize() / 2.f + text.rotationPoint;

            renderer.cache(sprite);

            sprite = hppv::Sprite(text);
            sprite.color = {0.4f, 0.2f, 0.4f, 0.f};

            renderer.cache(sprite);

            renderer.setShader(hppv::Render::Font);
            renderer.setTexture(font_.getTexture());
            renderer.cache(text);
        }
        {
            renderer.setShader(hppv::Render::CircleColor);
            hppv::Circle circle;
            circle.color = {0.f, 1.f, 1.f, 0.2f};
            circle.center = {20.f, 50.f};
            circle.radius = {3.f};
            renderer.cache(circle);
        }
        {
            renderer.setShader(hppv::Render::Tex);
            renderer.setTexture(texture_);
            hppv::Sprite sprite;
            sprite.pos = {80.f, 80.f};
            sprite.size = {100.f, 100.f};
            sprite.texRect = {0.f, 0.f, texture_.getSize()};
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
