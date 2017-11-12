#include <glm/gtc/constants.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>

class PixelShadows: public hppv::PrototypeScene
{
public:
    PixelShadows():
        hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {}

private:

    void prototypeRender(hppv::Renderer& renderer) override
    {
        using hppv::Sprite;

        renderer.shader(hppv::Render::Color);
        renderer.antialiasedSprites(true);

        {
            Sprite sprite(prototype_.space);
            sprite.color = {0.9f, 0.5f, 0.3f, 1.f};
            renderer.cache(sprite);
        }
        {
            const glm::vec4 color = {0.3f, 0.f, 0.f, 1.f};

            Sprite sprite;
            sprite.color = color;
            sprite.pos = {10.f, 10.f};
            sprite.size = {30.f, 8.f};
            sprite.rotation = glm::pi<float>() / 8.f;

            renderer.cache(sprite);

            sprite.pos = {50.f, 50.f};
            sprite.size = {40.f, 40.f};
            sprite.rotation = -0.1f;

            renderer.cache(sprite);
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
