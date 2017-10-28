#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>
#include <hppv/Texture.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Font.hpp>
#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>

class TestScene: public hppv::PrototypeScene
{
public:
    TestScene():
        PrototypeScene(hppv::Space(20.f, 20.f, 100.f, 100.f), 1.1f, false),
        texture_("/home/mati/font.png"),
        font_("/home/mati/font.fnt"),
        fb(GL_RGBA8, 1)
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
        //properties_.maximize = false;

        hppv::App::hideCursor(true);
    }

private:
    hppv::Texture texture_;
    hppv::Font font_;
    hppv::Framebuffer fb;
    bool lock_ = false;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::ShowTestWindow();

            {
                fb.bind();
                fb.setSize(properties_.size);
                fb.clear();
                //renderer.setViewport(fb.getSize());
            }
            //*/

            {
                renderer.setShader(hppv::RenderMode::color);
                hppv::Sprite sprite;
                sprite.color = {1.f, 1.f, 1.f, 0.5f};
                sprite.pos = {20.f, 20.f};
                sprite.size = {100.f, 100.f};
                renderer.cache(sprite);
            }
            {
                renderer.setShader(hppv::RenderMode::texture);
                renderer.setTexture(font_.getTexture());
                hppv::Sprite sprite;
                //sprite.color = {1.f, 0.f, 1.f, 0.4f};
                sprite.pos = {45.f, 45.f};
                sprite.size = {30.f, 30.f};
                sprite.texCoords = {0, 0, texture_.getSize()};
                renderer.cache(sprite);

                renderer.setShader(hppv::RenderMode::font);
                renderer.setTexture(font_.getTexture());
                hppv::Text text;
                text.text = "Hula dupal\nBarcelona !!! :D";
                text.font = &font_;
                text.pos = {0.f, 0.f};
                text.scale = 2.f;
                renderer.cache(text);

                auto viewport = hppv::spaceToWindow(hppv::Space(20.f, 20.f, 40.f, 40.f),
                                 hppv::expandToMatchAspectRatio(getSpace(),
                                     properties_.size), *this);

                renderer.setViewport({viewport.x, viewport.y}, {viewport.z, viewport.w},
                        frame_.framebufferSize);

                renderer.setProjection(hppv::Space(0.f, 0.f, 40.f, 40.f));
                renderer.setShader(hppv::RenderMode::circleColor);
                hppv::Circle circle;
                circle.center = {20.f, 20.f};
                circle.radius = 20.f;
                circle.color = {1.f, 0.5f, 0.f, 1.f};
                renderer.cache(circle);
            }

            renderer.flush();
            fb.unbind();
            renderer.setViewport(*this);

        renderer.setTexture(fb.getTexture(0));
        renderer.setShader(hppv::RenderMode::flippedY);

        hppv::Sprite sprite;
        sprite.pos = {0.f, 0.f};
        sprite.size = fb.getSize();
        sprite.texCoords = glm::ivec4(0, 0, fb.getSize());
        sprite.color = {1.f, 1.f, 0.f, 1.f};

        renderer.setProjection(hppv::Space(0.f, 0.f, fb.getSize().x, fb.getSize().y));

        renderer.cache(sprite);
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
