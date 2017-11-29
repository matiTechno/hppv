#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/imgui.h>

#include "Emitter.hpp"

class Particles: public hppv::PrototypeScene
{
public:
    Particles():
        hppv::PrototypeScene({0.f, 0.f, 1000.f, 1000.f}, 1.05f, false),
        emitter_(generator_)
    {
        std::random_device rd;
        generator_.seed(rd());
        configureEmitter();
        emitter_.reserveMemory();
    }

private:
    std::mt19937 generator_;
    Emitter emitter_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        emitter_.update(frame_.time);
        emitter_.render(renderer);

        ImGui::Begin("Particles", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        {
            ImGui::Text("count: %lu", emitter_.getCount());
            int hz = emitter_.spawn.hz;
            ImGui::SliderInt("spawn.hz", &hz, 60, 100000);
            emitter_.spawn.hz = hz;
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
    {
        auto pos = space_.initial.pos;
        auto size = space_.initial.size;
        emitter_.spawn.size = {size.x, 10.f};
        emitter_.spawn.pos = {pos.x, pos.y + size.y - emitter_.spawn.size.y - 20.f};
    }

    emitter_.spawn.hz = 10000;
    emitter_.life = {1.2f, 3.f};
    emitter_.vel = {{-2.f, -150.f}, {2.f, -200.f}};
    emitter_.acc = {{-50.f, -2.f}, {5.f, -50.f}};
    emitter_.radius = {1.f, 6.f};
    emitter_.colorStart = {{1.f, 0.f, 0.f, 0.f}, {1.f, 0.5f, 0.1f, 0.3f}};
    emitter_.colorEnd = {{0.25f, 0.25f, 0.25f, 0.4f}, {0.1f, 0.1f, 0.1f, 1.f}};
}
