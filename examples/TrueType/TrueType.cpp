#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Font.hpp>

class TrueType: public hppv::Scene
{
public:
    TrueType():
        font_("SourceCodePro-Regular.otf", 26)
    {
        properties_.maximize = true;
    }

    void render(hppv::Renderer& renderer)
    {
        renderer.projection({0, 0, properties_.size});
        renderer.texture(font_.getTexture());

        hppv::Text text(font_);
        text.pos = {100.f, 100.f};
        text.color = {0.f, 1.f, 0.f, 1.};
        text.text = "|!...{Hello World!\n"
                    "This is SourceCodePro-Regular.otf\n"
                    "rendered with stb_truetype.h";

        {
            renderer.shader(hppv::Render::Color);

            hppv::Sprite sprite(text);
            sprite.color = {0.1f, 0.f, 0.f, 1.f};

            renderer.cache(sprite);
        }

        renderer.shader(hppv::Render::Font);
        renderer.cache(text);

        {
            renderer.shader(hppv::Render::Tex);

            hppv::Sprite sprite;
            sprite.texRect = {0, 0, font_.getTexture().getSize()};
            sprite.pos = {100.f, 300.f};
            sprite.size = font_.getTexture().getSize();

            renderer.cache(sprite);
        }

        renderer.shader(hppv::Render::Font);

        text.text = "SCALED!!!\n{  ... :D}";
        text.scale = 3.f;
        text.pos = {300.f, 500.f};

        renderer.cache(text);
    }

private:
    hppv::Font font_;
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<TrueType>();
    app.run();
    return 0;
}
