#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

class TestScene: public PrototypeScene
{
public:
    TestScene():
        PrototypeScene(Space(0.f, 0.f, 100.f, 100.f), 1.1f, false)
    {
        properties_.pos = {100, 100};
        properties_.size = {500, 200};
        properties_.maximize = false;
    }

private:
    void prototypeRender(Renderer& renderer) override
    {
        ImGui::ShowTestWindow();
        {
            Sprite sprite;
            sprite.color = {0.f, 1.f, 0.f, 0.1f};
            sprite.pos = {0.f, 0.f};
            sprite.size = {100.f, 100.f};
            renderer.cache(sprite);
        }
        {
            Sprite sprite;
            sprite.color = {1.f, 0.f, 0.f, 0.8f};
            sprite.pos = {25.f, 25.f};
            sprite.size = {50.f, 50.f};
            renderer.cache(sprite);
        }
    }
};

int main()
{
    App app;
    if(!app.initialize()) return 1;
    app.executeScene<TestScene>();
    return 0;
}
