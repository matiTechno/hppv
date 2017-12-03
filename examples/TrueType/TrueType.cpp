#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Font.hpp>

class TrueType: public hppv::Scene
{
public:
    TrueType():
        font1_("SourceCodePro-Regular.otf", 26, "ĄąĆćĘęŁłŃńÓóŚśŹźŻż"),
        font2_("ProggyClean.ttf", 13)
    {
        properties_.maximize = true;
    }

    void render(hppv::Renderer& renderer)
    {
        renderer.projection({0, 0, properties_.size});
        renderer.texture(font1_.getTexture());

        hppv::Text text(font1_);
        text.pos = {10.f, 10.f};
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
            sprite.texRect = {0, 0, font1_.getTexture().getSize()};
            sprite.pos = {100.f, 400.f};
            sprite.size = font1_.getTexture().getSize();

            renderer.cache(sprite);
        }

        renderer.shader(hppv::Render::Font);

        text.pos = {20.f, 100.f};
        text.color = {1.f, 0.f, 1.f, 1.f};
        text.text = "Wiatr drzewa spienia. Ziemia dojrzała.\n"
                    "Kłosy brzuch ciężki w górę unoszą\n"
                    "i tylko chmury - palcom lub włosom\n"
                    "podobne - suną drapieżnie w mrok.\n\n"
                    "Ziemia owoców pełna po brzegi\n"
                    "kipi sytością jak wielka misa.\n"
                    "Tylko ze świerków na polu zwisa\n"
                    "głowa obcięta strasząc jak krzyk.\n"
                    "... Krzysztof Kamil Baczyński";

        renderer.cache(text);

        text.pos = {350.f, 400.f};
        text.scale = 3.f;
        text.color = {0.f, 1.f, 0.f, 1.f};
        text.text = "SCALED!!!\n{  ... :D}";

        renderer.cache(text);

        renderer.texture(font2_.getTexture());

        text.pos.y += text.getSize().y;
        text.font = &font2_;
        text.scale = 1.f;
        text.color = {1.f, 0.5f, 0.f, 1.f};
        text.text = "#include <iostream>\n"
                    "int main()\n"
                    "{\n"
                    "    std::cout << \"Hello World!\" << std::endl;\n"
                    "    return 0;\n"
                    "}";

        renderer.cache(text);
    }

private:
    hppv::Font font1_, font2_;
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<TrueType>();
    app.run();
    return 0;
}
