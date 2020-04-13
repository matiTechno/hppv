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

float signal2(float t)
{
    if(t < 5.f)
        return 5.f;
    else
        return 1.f;
}

struct Sine
{
    float amp;
    float fq;
    float phase;
};

float signal1(float t)
{
    Sine sines[] = {
        {2.f, 0.02f, pi / 10.f},
        {1.f, 0.1f, pi / 2.f},
        {0.5f, 0.2f, pi / 3.f},
        {0.4f, 1.f, pi / 6.f},
        {1.f, 3.3f, pi / 9.f},
        {0.02f, 5.f, 0.f},
    };

    float c = 2.f * pi * t;
    float result = 0.f;

    for(Sine sine: sines)
        //result += sine.amp * sinf(c * sine.fq + sine.phase);
        result += sinf(c * sine.fq);
    return result;
}

// Fourier series coeff
struct Coeff
{
    float module; // to recover an amplitude divide it by the number of harmonics
    float phase;
};

// discrete Fourier transform

Coeff dft(std::vector<float> signal, int freq)
{
    assert(freq <= signal.size() / 2); // don't exceed Nyquist frequency

    int n = 0;
    float re = 0.f;
    float im = 0.f;

    for(float s: signal)
    {
        float angle = -2.f * pi * freq * (float)n / signal.size();
        re += s * cosf(angle);
        im += s * sinf(angle);
        n += 1;
    }
    Coeff coeff;
    coeff.module = sqrt(re * re + im * im);
    coeff.phase = atan2f(im, re);
    return coeff;
}

void draw_axes(hppv::Renderer& rr, glm::vec2 origin, float line_length)
{
    rr.breakBatch();
    rr.mode(hppv::RenderMode::Vertices);
    rr.shader(hppv::Render::VerticesColor);
    rr.primitive(GL_LINES);
    float offset = line_length / 2.f;
    hppv::Vertex v;
    v.pos = origin + glm::vec2(-offset, 0.f);
    rr.cache(&v, 1);
    v.pos = origin + glm::vec2(offset, 0.f);
    rr.cache(&v, 1);
    v.pos = origin + glm::vec2(0.f, -offset);
    rr.cache(&v, 1);
    v.pos = origin + glm::vec2(0.f, offset);
    rr.cache(&v, 1);
    rr.breakBatch();
}

class Numerics: public hppv::Prototype
{
public:
    bool taylor_mode = false;
    int taylor_degree = 1;
    std::vector<float> signal;
    float sampling_period = 0.005f;
    float total_time = 15.f;
    std::vector<Coeff> coeffs;
    std::vector<float> synthesis; // reconstructed function using sines and coefficients

    Numerics():
        hppv::Prototype({-10.f, -10.f, 40.f, 40.f})
    {
        for(float t = 0.f; t < total_time; t += sampling_period)
            signal.push_back(signal1(t));

        for(int i = 0; i < (int)signal.size() / 2; ++i) // divided by 2 to not exceed Niquist frequency
            coeffs.push_back(dft(signal, i));

        for(int n = 0; n < (int)signal.size(); ++n)
        {
            float sum = 0.f;
            int freq = 0;

            for(Coeff coeff: coeffs)
            {
                float angle = (pi / 2.f + coeff.phase) + (2.f * pi * freq * n / signal.size());
                sum += coeff.module * sinf(angle);
                freq += 1;
            }
            sum = sum / (float)coeffs.size();
            synthesis.push_back(sum);
        }
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::Checkbox("show taylor", &taylor_mode);
        ImGui::InputInt("taylor degree", &taylor_degree);
        ImGui::End();

        if(taylor_mode)
        {
            draw_axes(rr, {0.f, 0.f}, 20.f);
            rr.primitive(GL_POINTS);

            for(float x = -3.f * pi; x < 3.f * pi; x += 0.01f)
            {
                float y = -sinf(x);
                hppv::Vertex v;
                v.pos = {x, y};
                rr.cache(&v, 1);
                v.color = {1.f, 1.f, 0.f, 1.f};
                v.pos.y = -sine_taylor(x, taylor_degree);
                rr.cache(&v, 1);
            }
            return;
        }

        // Fourier mode

        // time domain

        draw_axes(rr, {0.f, 0.f}, 20.f);
        rr.primitive(GL_LINE_STRIP);
        float t = 0.f;

        for(float s: signal)
        {
            hppv::Vertex v;
            v.pos = {t, -s};
            v.color = {0.f, 0.5f, 0.f, 1.f};
            rr.cache(&v, 1);
            t += sampling_period;
        }

        // frequency domain

        float yoff = 10.f;
        draw_axes(rr, {0.f, yoff}, 20.f);
        rr.primitive(GL_LINE_STRIP);

        for(int i = 0; i < coeffs.size(); ++i)
        {
            float freq = (float)i / (signal.size() * sampling_period);
            float amp = coeffs[i].module / coeffs.size();
            hppv::Vertex v;
            v.color = {1.f, 1.f, 0.f, 1.f};
            v.pos.x = freq;
            v.pos.y = yoff + -amp;
            rr.cache(&v, 1);
        }

        // synthesis; Fourier series sum

        yoff = 20.f;
        draw_axes(rr, {0.f, yoff}, 20.f);
        rr.primitive(GL_LINE_STRIP);
        t = 0.f;

        for(float s: synthesis)
        {
            hppv::Vertex v;
            v.pos = {t, yoff + -s};
            v.color = {0.f, 0.5f, 0.f, 1.f};
            rr.cache(&v, 1);
            t += sampling_period;
        }
    }
};

RUN(Numerics)
