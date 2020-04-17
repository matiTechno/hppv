#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <atomic>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>
#include <hppv/glad.h>

#include "../run.hpp"

#define pi (3.14159f)

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

#define SAMPLE_COUNT 4096
#define AUDIO_FQ 44100

struct AudioStream
{
    unsigned char* buf;
    unsigned int size;
    std::atomic_int pos;
};

void audio_callback(void* _userdata, unsigned char* dst, int size)
{
    assert(size == 2 * 2 * SAMPLE_COUNT); // 2 channels, 2 bytes per sample
    AudioStream* stream = (AudioStream*)_userdata;
    assert(stream->size >= size);

    if(stream->pos + size > stream->size)
        stream->pos = 0;

    memcpy(dst, stream->buf + stream->pos, size);
    stream->pos += size;
}

float map(float x, float to_beg, float to_end, float from_beg, float from_end)
{
    return to_beg + (x / (from_end - from_beg)) * (to_end - to_beg);
}

class AudioFFT: public hppv::Prototype
{
public:
    AudioStream stream;
    std::vector<Complex> fft_data;

    AudioFFT(): hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {
        fft_data.resize(SAMPLE_COUNT);
        stream.pos = 0;
        int r = SDL_Init(SDL_INIT_AUDIO);
        assert(!r);
        SDL_AudioSpec spec;

        if(!SDL_LoadWAV("/home/mat/output.wav", &spec, &stream.buf, &stream.size))
        {
            printf("error: %s\n", SDL_GetError());
            assert(false);
        }
        assert(spec.channels == 2);
        assert(spec.freq == AUDIO_FQ);
        assert(spec.samples == SAMPLE_COUNT); // audio buffer size
        assert(spec.format == AUDIO_S16LSB);
        spec.userdata = &stream;
        spec.callback = audio_callback;
        SDL_AudioSpec spec_used;
        SDL_AudioDeviceID devid = SDL_OpenAudioDevice(nullptr, false, &spec, &spec_used, 0);

        if(!devid)
        {
            printf("error: %s\n", SDL_GetError());
            assert(false);
        }
        SDL_PauseAudioDevice(devid, 0);
    }

    ~AudioFFT()
    {
        SDL_Quit();
    }

    void prototypeRender(hppv::Renderer& rr)
    {
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::End();

        int16_t* src = (int16_t*)(stream.buf + stream.pos);

        for(int i = 0; i < SAMPLE_COUNT; ++i)
        {
            // *2 to skip the second channel (sound is stereo); + normalize
            float amp = (float)src[i * 2] / ((1 << 15) - 1);
            fft_data[i] = {amp, 0};
        }

        fft(fft_data.data(), fft_data.size());

        rr.primitive(GL_LINE_STRIP);
        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);

        for(int k = 1; k < fft_data.size() / 2; ++k) // skip a DC component and frequencies above the Nyquist fq
        {
            Complex c = fft_data[k];
            float amp = sqrtf(c.re * c.re + c.im * c.im) * 2.f / fft_data.size();
            float real_fq = (float)(k * AUDIO_FQ) / fft_data.size();
            float x = log10f(real_fq);
            x = map(x, 0.f, 100.f, 0.f, 5.f);
            hppv::Vertex v;
            v.pos.x = x;
            v.pos.y = 80.f + -120.f * amp;
            rr.cache(&v, 1);
        }
    }
};

RUN(AudioFFT)
