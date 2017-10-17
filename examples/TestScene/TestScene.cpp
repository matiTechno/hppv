#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>
#include <hppv/Texture.hpp>

class TestScene: public hppv::PrototypeScene
{
public:
    TestScene():
        PrototypeScene(hppv::Space(20.f, 20.f, 100.f, 100.f), 1.1f, false),
        texture("cocacola.png")
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
        properties_.maximize = false;
    }

private:
    hppv::Texture texture;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::ShowTestWindow();
        {
            hppv::Sprite sprite;
            sprite.color = {0.f, 1.f, 0.f, 0.1f};
            sprite.pos = {20.f, 20.f};
            sprite.size = {100.f, 100.f};
            //renderer.cache(sprite);
        }
        {
            hppv::Sprite sprite;
            //sprite.color = {1.f, 0.f, 0.f, 0.8f};
            sprite.pos = {45.f, 45.f};
            sprite.size = {30.f, 30.f};
            sprite.texture = &texture;
            sprite.texCoords = {0, 0, texture.getSize()};
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
