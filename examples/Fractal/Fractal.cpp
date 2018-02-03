#include <algorithm>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

// todo: colors, double precision
const char* const fractalSource = R"(

#fragment
#version 330

in vec2 vPos;

uniform vec2 projPos;
uniform vec2 projSize;
uniform int iterations;

out vec4 color;

void main()
{
    vec2 pos = projPos + vPos * projSize;
    vec2 z = vec2(0.0);
    int it = 0;

    while(pow(z.x, 2.0) + pow(z.y, 2.0) < 4.0 && it < iterations)
    {
        float xtemp = pow(z.x, 2.0) - pow(z.y, 2.0) + pos.x;
        z.y = 2.0 * z.x * z.y + pos.y;
        z.x = xtemp;
        ++it;
    }

    color = vec4(float(it) / iterations);
}
)";

class Fractal: public hppv::Prototype
{
public:
    Fractal():
        hppv::Prototype({-2.5f, -1.f, 3.5f, 2.f}),
        sh_({hppv::Renderer::vInstancesSource, fractalSource}, "sh_")
    {
        prototype_.alwaysZoomToCursor = true;
    }

private:
    hppv::Shader sh_;
    int iterations_ = 100;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::InputInt("iterations", &iterations_);
        iterations_= std::max(0, iterations_);
        ImGui::End();

        const auto proj = space_.projected;

        sh_.bind();
        sh_.uniform2f("projPos", proj.pos);
        sh_.uniform2f("projSize", proj.size);
        sh_.uniform1i("iterations", iterations_);

        renderer.shader(sh_);

        renderer.cache(hppv::Sprite(proj));
    }
};

RUN(Fractal)
