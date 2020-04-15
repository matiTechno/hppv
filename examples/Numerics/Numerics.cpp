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

struct Sine
{
    float amp;
    float fq;
    float phase;
};

float signal1(float t)
{
    Sine sines[] = {
        {2.f, 0.2f, pi},
        {1.f, 0.3f, pi / 2.f},
        {0.5f, 0.5f, pi / 3.f},
        {0.4f, 1.f, pi / 6.f},
        {0.5f, 3.3f, pi / 9.f},
        {0.1f, 5.f, pi * 1.5f},
    };

    float c = 2.f * pi * t;
    float result = 1.59f; // some arbitrary offset

    for(Sine sine: sines)
        result += sine.amp * sinf(c * sine.fq + sine.phase);
    return result;
}

float signal2(float t)
{
    if(t < 5.f)
        return 5.f;
    else
        return 1.f;
}

float signal3(float t)
{
    return 0.2f * t * t + 3.f * t - 2.f;
}

float signal4(float t)
{
    return expf(t) * 0.1f + 0.22f;
}

struct Harmonic
{
    float amp;
    float phase;
};

// discrete Fourier transform with some post-processing (converts Fourier coefficient module to a proper amplitude of a harmonic)
// returns a vector of harmonic sines; sine at index 0 is a DC component (signal mean)
// http://www.dspguide.com/ch8/3.htm

std::vector<Harmonic> dft(std::vector<float> signal)
{
    assert(signal.size());
    assert(signal.size() % 2 == 0); // I don't know if it matters here, probably not that much
    std::vector<Harmonic> harmonics;

    for(int k = 0; k <= signal.size() / 2; ++k) // note: <= is used, not <; signal.size() / 2 is the Nyquist frequency, frequencies above it are skipped
    {
        int n = 0;
        float re = 0.f;
        float im = 0.f;

        for(float x: signal)
        {
            float angle = -2.f * pi * k * n / signal.size();
            re += x * cosf(angle);
            im += x * sinf(angle);
            n += 1;
        }
        Harmonic hm;
        hm.phase = pi / 2.f + atan2f(im, re); // pi/2 is used to translate cos into sin
        hm.amp = sqrtf(re * re + im * im) / signal.size();

        if(k != 0 && k != signal.size() / 2) // some explanation for this is in dspguide chapter 8
            hm.amp *= 2;
        harmonics.push_back(hm);
    }
    return harmonics;
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

#define TEST_SIZE 200

class Numerics: public hppv::Prototype
{
public:
    bool taylor_mode = false;
    int taylor_degree = 1;
    std::vector<float> signal;
    float sampling_period = 0.005f;
    std::vector<Harmonic> harmonics;
    std::vector<float> synthesis; // reconstructed signal using Fourier series (harmonics)
    struct
    {
        int freq = 1;
        float time = 1.f;
        bool play_time = true;
        float mean_dist;
        bool show_signal_sum = true;

    } debug;

    Numerics():
        hppv::Prototype({-10.f, -10.f, 40.f, 40.f})
    {
        float total_time = 30.f; // in case of an exponential function signal change this to a smaller value, e.g. 5.f (otherwise DFT will produce some crap)

        for(float t = 0.f; t < total_time; t += sampling_period)
            signal.push_back(signal1(t)); // change signal function to see the effect on various input functions

        if(signal.size() % 2)
            signal.pop_back();

        harmonics = dft(signal);

        for(int n = 0; n < signal.size(); ++n)
        {
            float sum = 0.f;

            for(int k = 0; k < harmonics.size(); ++k)
            {
                Harmonic hm = harmonics[k];
                float angle = hm.phase + (2.f * pi * k * n / signal.size());
                sum += hm.amp * sinf(angle);
            }
            synthesis.push_back(sum);
        }
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::Checkbox("show taylor", &taylor_mode);
        ImGui::InputInt("taylor degree", &taylor_degree);
        ImGui::SliderInt("winding frequency", &debug.freq, 0, TEST_SIZE - 1);
        ImGui::SliderFloat("time", &debug.time, 0.f, 1.f);
        ImGui::Checkbox("play time", &debug.play_time);
        ImGui::Checkbox("show signal sum", &debug.show_signal_sum);
        ImGui::Text("mean distance: %f", debug.mean_dist);
        ImGui::End();

        if(debug.play_time)
        {
            if(debug.time > 1.f)
                debug.time = 0.f;

            debug.time += frame_.time / 5.f;
        }

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

        for(int k = 1; k < harmonics.size(); ++k) // k = 1, skip the DC component
        {
            float freq = (float)k / (signal.size() * sampling_period);
            hppv::Vertex v;
            v.color = {1.f, 1.f, 0.f, 1.f};
            v.pos.x = freq;
            v.pos.y = yoff + -harmonics[k].amp;
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

        // some stuff to help me understand how DFT works

        float test_signal[TEST_SIZE];
        float part1[TEST_SIZE];
        float part2[TEST_SIZE];

        for(int n = 0; n < TEST_SIZE; ++n)
        {
            part1[n] = 2.f * sinf( (2.f * pi * n / TEST_SIZE * 5.f) + (pi * 0.8f) );
            part2[n] = 1.f * sinf( (2.f * pi * n / TEST_SIZE * 10.f) + (pi * 0.3f) );
            test_signal[n] = part1[n] + part2[n] + 0.2f;
        }

        float re_list[TEST_SIZE];
        float im_list[TEST_SIZE];
        float sum_re = 0.f;
        float sum_im = 0.f;

        for(int n = 0; n < TEST_SIZE; ++n)
        {
            float angle = -2.f * pi * n / TEST_SIZE * debug.freq;
            re_list[n] = test_signal[n] * cosf(angle);
            im_list[n] = test_signal[n] * sinf(angle);
            sum_re += re_list[n];
            sum_im += im_list[n];
        }
        float module = sqrt(sum_re * sum_re + sum_im * sum_im);
        debug.mean_dist = module / TEST_SIZE;

        // draw a signal

        yoff = 30.f;
        draw_axes(rr, {0.f, yoff}, 20.f);
        rr.primitive(GL_LINE_STRIP);
        int end = TEST_SIZE * debug.time;

        if(debug.show_signal_sum)
        {
            for(int n = 0; n < end; ++n)
            {
                hppv::Vertex v;
                v.pos = {10.f * n / TEST_SIZE, yoff + -test_signal[n]};
                rr.cache(&v, 1);
            }
        }
        else
        {
            for(int n = 0; n < end; ++n)
            {
                hppv::Vertex v;
                v.color = {0.5f, 0.5f, 0.f, 1.f};
                v.pos = {10.f * n / TEST_SIZE, yoff + -part1[n]};
                rr.cache(&v, 1);
            }
            rr.breakBatch();

            for(int n = 0; n < end; ++n)
            {
                hppv::Vertex v;
                v.color = {0.f, 0.5f, 0.f, 1.f};
                v.pos = {10.f * n / TEST_SIZE, yoff + -part2[n]};
                rr.cache(&v, 1);
            }
        }

        // draw the signal on a circle

        yoff = 40.f;
        draw_axes(rr, {0.f, yoff}, 20.f);
        rr.primitive(GL_LINE_STRIP);

        for(int n = 0; n < end; ++n)
        {
            hppv::Vertex v;
            v.color = {0.3f, 0.3f, 0.f, 0.f};

            if(n == end - 1)
                v.color = {1.f, 0.f, 0.f, 1.f};
            v.pos = {re_list[n], yoff + -im_list[n]};
            rr.cache(&v, 1);
        }

        // draw a center of 'mass' of the signal

        rr.mode(hppv::RenderMode::Instances);
        rr.shader(hppv::Render::CircleColor);
        hppv::Circle c;
        c.radius = 0.2f;
        c.color = {1.f, 0.2f, 0.2f, 1.f};
        c.center = {sum_re / TEST_SIZE, yoff + -sum_im / TEST_SIZE};
        rr.cache(c);
    }
};

RUN(Numerics)
