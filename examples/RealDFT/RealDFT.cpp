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

int ilog2(int n)
{
    return log2(n) + 0.5f;
}

struct Complex
{
    float re;
    float im;
};

Complex cx_exp(float exp)
{
    return {cosf(exp), sinf(exp)};
}

Complex cx_mul(Complex lhs, Complex rhs)
{
    return {lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + lhs.im * rhs.re};
}

Complex cx_add(Complex lhs, Complex rhs)
{
    return {lhs.re + rhs.re, lhs.im + rhs.im};
}

Complex cx_sub(Complex lhs, Complex rhs)
{
    return {lhs.re - rhs.re, lhs.im - rhs.im};
}

void rfft_impl(Complex* x, int N)
{
    if(N == 1)
        return;

    int n = N / 2;
    rfft_impl(x, n);
    rfft_impl(x + n, n);

    for(int k = 0; k < n; ++k)
    {
        Complex even = x[k];
        Complex odd = x[k + n];
        Complex rhs = cx_mul(cx_exp(-2.f * pi * k / N), odd);
        x[k] = cx_add(even, rhs);
        x[k + n] = cx_sub(even, rhs);
    }
}

// bit-reversal sorting

void fft_sort(Complex* x, int N)
{
    assert( ((N & (N - 1)) == 0) && N); // N must be a power of two

    for(unsigned int i = 0; i < N; ++i)
    {
        assert(sizeof(unsigned int) == 4);

        unsigned int b = i;

        b = ((b >> 1) & 0x55555555) | ((b & 0x55555555) << 1);
        b = ((b >> 2) & 0x33333333) | ((b & 0x33333333) << 2);
        b = ((b >> 4) & 0x0f0f0f0f) | ((b & 0x0f0f0f0f) << 4);
        b = ((b >> 8) & 0x00ff00ff) | ((b & 0x00ff00ff) << 8);
        b = ((b >> 16) | (b << 16)) >> (32 - ilog2(N));

        if(b > i)
        {
            Complex tmp = x[i];
            x[i] = x[b];
            x[b] = tmp;
        }
    }
}

// recursive in-place FFT implementation
// x is an array of samples and is overwritten with output (Fourier coefficients; or inversely when used as an inverse transformation)

void rfft(Complex* x, int N)
{
    fft_sort(x, N);
    rfft_impl(x, N);
}

// iterative implementation

void fft(Complex* x, int N)
{
    fft_sort(x, N);

    for(int stride = 2; stride <= N; stride *= 2)
    {
        Complex w_s = cx_exp(-2.f * pi / stride);
        int n = stride / 2;

        for(int off = 0; off < N; off += stride)
        {
            Complex w = {1.f, 0.f};

            for(int k = 0; k < n; ++k)
            {
                Complex even = x[off + k];
                Complex odd = x[off + k + n];
                Complex rhs = cx_mul(w, odd);
                x[off + k] = cx_add(even, rhs);
                x[off + k + n] = cx_sub(even, rhs);
                w = cx_mul(w, w_s);
            }
        }
    }
}

#define TEST_SIZE 200

class RealDFT: public hppv::Prototype
{
public:
    bool taylor_mode = false;
    int taylor_degree = 1;
    std::vector<float> signal;
    float sampling_period;
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

    RealDFT():
        hppv::Prototype({-10.f, -10.f, 40.f, 40.f})
    {
        float total_time = 30.f; // in case of an exponential function signal change this to a smaller value, e.g. 5.f (otherwise DFT will produce some crap)

        int sample_count = 1024; // must be a power of two (for FFT)
        sampling_period = total_time / sample_count;

        float t = 0.f;

        for(int i = 0; i < sample_count; ++i)
        {
            signal.push_back(signal1(t)); // change signal function to see the effect on various input functions
            t += sampling_period;
        }

        // test FFT implementation of DFT
        {
            std::vector<Complex> complex_signal;

            for(float s: signal)
                complex_signal.push_back({s, 0.f});

            fft(complex_signal.data(), complex_signal.size()); // alternatively use a recursive version

            // Fourier coefficients are stored in complex_signal, transform them into harmonics

            for(int k = 0; k <= complex_signal.size() / 2; ++k) // notice <=, same as in realDFT(); don't exceed Nyquist frequency
            {
                Complex coeff = complex_signal[k];
                Harmonic hm;
                hm.amp = sqrt(coeff.re * coeff.re + coeff.im * coeff.im) / complex_signal.size();
                hm.phase = pi / 2.f + atan2f(coeff.im, coeff.re);

                if(k != 0 && k != complex_signal.size() / 2)
                    hm.amp *= 2.f;

                harmonics.push_back(hm);
            }

            // inverse transform (synthesis)
            // https://www.embedded.com/dsp-tricks-computing-inverse-ffts-using-the-forward-fft/

            for(Complex& c: complex_signal)
                c.im *= -1.f;

            fft(complex_signal.data(), complex_signal.size());
            // second negation is skipped, only real part is used

            for(Complex c: complex_signal)
                synthesis.push_back(c.re / complex_signal.size());
        }

        // disabled in favor of FFT
        /*
        harmonics = dft(signal);

        for(int n = 0; n < signal.size(); ++n)
        {
            float sum = 0.f;

            for(int k = 0; k < harmonics.size(); ++k) // try e.g. k < 40 to see intermediate approximation
            {
                Harmonic hm = harmonics[k];
                float angle = hm.phase + (2.f * pi * k * n / signal.size());
                sum += hm.amp * sinf(angle);
            }
            synthesis.push_back(sum);
        }
        */
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

RUN(RealDFT)
