#include <assert.h>
#include <math.h>
#include <vector>

#include <hppv/glad.h>
#include <hppv/Prototype.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>

#include "../run.hpp"

#define pi (3.14159265359f)

float sine_taylor(float x, int count)
{
    float dividend = x;
    float divisor = 1.f;
    float result = 0.f;

    for(int i = 0; i < count; ++i)
    {
        result += dividend / divisor;
        dividend = dividend * x * x * -1.f;
        divisor = divisor * (divisor + 1.f) * (divisor + 2.f);
    }
    return result;
}

class Numerics: public hppv::Prototype
{
public:
    Numerics():
        hppv::Prototype({-10.f, -10.f, 20.f, 20.f})
    {
    }

    int n = 1;

    void prototypeRender(hppv::Renderer& rr) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::InputInt("n", &n);
        ImGui::End();

        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);

        rr.primitive(GL_LINES);
        {
            hppv::Vertex v;
            v.color = {0.f, 0.5f, 0.f, 1.f};
            v.pos = {0.f, -10.f};
            rr.cache(&v, 1);
            v.pos.y += 20.f;
            rr.cache(&v, 1);
            v.pos = {-10.f, 0.f};
            rr.cache(&v, 1);
            v.pos.x += 20.f;
            rr.cache(&v, 1);
        }

        rr.primitive(GL_POINTS);

        for(float x = -3.f * pi; x < 3.f * pi; x += 0.01f)
        {
            float y = -sinf(x);
            hppv::Vertex v;
            v.pos = {x, y};
            rr.cache(&v, 1);
            v.color = {1.f, 1.f, 0.f, 1.f};
            v.pos.y = -sine_taylor(x, n);
            rr.cache(&v, 1);
        }
    }
};

RUN(Numerics)
