#include <hppv/Prototype.hpp>
#include <hppv/imgui.h>

#include "Emitter.hpp"
#include "../run.hpp"

// note: rendering lots of particles in one place might slow down OpenGL (overdraw)

class Particles: public hppv::Prototype
{
public:
    Particles():
        hppv::Prototype({0.f, 0.f, 1000.f, 1000.f}),
        emitter_(generator_)
    {
        prototype_.alwaysZoomToCursor = false;

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

        ImGui::Begin(prototype_.imguiWindowName);
        {
            ImGui::Text("count: %lu", emitter_.getCount());
            ImGui::SliderInt("spawn.hz", &emitter_.spawn.hz, 60, 100000);
        }
        ImGui::End();
    }

    void configureEmitter()
    {
        {
            const auto pos = space_.initial.pos;
            const auto size = space_.initial.size;
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
};

RUN(Particles)
