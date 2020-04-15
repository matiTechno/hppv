#include <vector>
#include <math.h>
#include <stdlib.h>

#include <hppv/glad.h>
#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

glm::vec2 get_spline_point(std::vector<glm::vec2> control_points, float t)
{
    assert(control_points.size() >= 4);
    int id1 = (int)t;
    assert(id1 < control_points.size());

    int id0 = id1 - 1;

    if(id0 < 0)
        id0 = control_points.size() - 1;

    int id2 = id1 + 1;

    if(id2 == control_points.size())
        id2 = 0;

    int id3 = id2 + 1;

    if(id3 == control_points.size())
        id3 = 0;

    glm::vec2 p0 = control_points[id0];
    glm::vec2 p1 = control_points[id1];
    glm::vec2 p2 = control_points[id2];
    glm::vec2 p3 = control_points[id3];

    t = t - (int)t; // cut the whole part

    float tt = t * t;
    float ttt = t * t * t;

    // 4 basis functions

    float b0 = -ttt + 2.f * tt - t;
    float b1 = 3.f * ttt - 5.f * tt + 2.f;
    float b2 = -3.f * ttt + 4.f * tt + t;
    float b3 = ttt - tt;

    return 0.5f * (p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3);
}

struct Complex
{
    float re;
    float im;
};

#define pi (3.14159f)

Complex complex_dft(std::vector<Complex> signal, int freq)
{
    int N = signal.size();
    Complex coeff;
    coeff.re = 0.f;
    coeff.im = 0.f;

    for(int n = 0; n < N; ++n)
    {
        float angle = -2.f * pi * freq * n / N;
        float re = cosf(angle);
        float im = sinf(angle);
        Complex sig = signal[n];
        // complex numbers multiplication
        coeff.re += sig.re * re - sig.im * im;
        coeff.im += sig.re * im + sig.im * re;
    }
    coeff.re /= N;
    coeff.im /= N;
    return coeff;
}

int cmp_arrows(const void* _lhs, const void* _rhs)
{
    glm::vec2 lhs = *(glm::vec2*)_lhs;
    glm::vec2 rhs = *(glm::vec2*)_rhs;
    float mod_lhs = lhs.x * lhs.x + lhs.y * lhs.y;
    float mod_rhs = rhs.x * rhs.x + rhs.y * rhs.y;

    if(mod_lhs < mod_rhs)
        return 1;
    return -1;
}

class FourierDrawing: public hppv::Prototype
{
public:
    std::vector<Complex> path;
    std::vector<Complex> coeffs;
    std::vector<glm::vec2> arrows; // 2d vectors
    float time = 0.f;
    int max_fq = 100;
    bool play_time = true;

    FourierDrawing(): hppv::Prototype({-30.f, -30.f, 130.f, 130.f})
    {
        std::vector<glm::vec2> control_points = {{10.f, 10.f}, {0.f, 0.f}, {-20.f, 40.f}, {0.f, 21.f}, {15.f, 80.f},
                                                 {30.f, 40.f}, {60.f, 60.f}, {60.f, 10.f}, {40.f, 30.f}, {20.f, -10.f}};
        float dt = control_points.size() / 200.f;
        float t = 0.f;

        while(t < control_points.size())
        {
            glm::vec2 p = get_spline_point(control_points, t);
            path.push_back({p.x, p.y});
            t += dt;
        }

        for(int i = -max_fq; i <= max_fq; ++i)
            coeffs.push_back(complex_dft(path, i));

        arrows.resize(coeffs.size());
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::Checkbox("play time", &play_time);
        ImGui::End();

        if(play_time)
            time += frame_.time / 20.f;

        if(time >= 1.f)
            time = 0.f;

        // inverse Fourier transform
        for(int i = 0; i < coeffs.size(); ++i)
        {
            float angle = 2.f * pi * time * (i - max_fq);
            float re = cosf(angle);
            float im = sinf(angle);
            // complex numbers multiplication
            arrows[i].x = coeffs[i].re * re - coeffs[i].im * im;
            arrows[i].y = coeffs[i].re * im + coeffs[i].im * re;
        }

        qsort(arrows.data(), arrows.size(), sizeof(glm::vec2), cmp_arrows);

        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);
        rr.primitive(GL_LINE_LOOP);

        for(Complex p: path)
        {
            hppv::Vertex v;
            v.color *= 0.3f;
            v.pos = {p.re, p.im};
            rr.cache(&v, 1);
        }

        rr.primitive(GL_LINE_STRIP);

        hppv::Vertex v;
        v.color = {1.f, 0.3f, 0.3f, 0.1f};
        v.pos = {0.f, 0.f};

        for(int i = 0; i < arrows.size(); ++i)
        {
            v.pos += arrows[i];
            rr.cache(&v, 1);
        }
    }
};

RUN(FourierDrawing)
