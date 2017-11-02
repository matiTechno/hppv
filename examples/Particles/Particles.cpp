#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/imgui.h>

#include "Emitter.hpp"

class Particles: public hppv::PrototypeScene
{
public:
    Particles():
        hppv::PrototypeScene(hppv::Space(0.f, 0.f, 1000.f, 1000.f), 1.05f, false)
    {
        std::random_device rd;
        generator_.seed(rd());
        emitter.generator = &generator_;
        configureEmitter();
    }

private:
    std::mt19937 generator_;
    Emitter emitter;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        emitter.update(frame_.frameTime);
        emitter.render(renderer);

        ImGui::Begin("Particles", nullptr, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
        {
            // set emitter_ properties; load / save
            ImGui::Text("count: %lu", emitter.getCount());
        }
        ImGui::End();
    }

    void configureEmitter();
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<Particles>();
    app.run();
    return 0;
}

void Particles::configureEmitter()
{
    auto& e = emitter;
    {
        auto pos = prototype_.initialSpace.pos;
        auto size = prototype_.initialSpace.size;
        e.spawn.size = {size.x, 10.f};
        e.spawn.pos = {pos.x, pos.y + size.y - emitter.spawn.size.y - 20.f};
    }

    e.spawn.hz = 10000;
    e.life = {1.2f, 3.f};
    e.vel = {{-2.f, -150.f}, {2.f, -200.f}};
    e.acc = {{-50.f, -2.f}, {5.f, -50.f}};
    e.radius = {1.f, 6.f};
    e.colorStart = {{1.f, 0.f, 0.f, 0.f}, {1.f, 0.5f, 0.1f, 0.3f}};
    e.colorEnd = {{0.25f, 0.25f, 0.25f, 0.4f}, {0.1f, 0.1f, 0.1f, 1.f}};
}
