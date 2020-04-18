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

void fft_sort(Complex* x, unsigned int N)
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

float map(float x, float to_beg, float to_end, float from_beg, float from_end)
{
    return to_beg + (x / (from_end - from_beg)) * (to_end - to_beg);
}

// size of audio buffer (in samples)
#define SAMPLE_COUNT 4096

#define AUDIO_FQ 44100

typedef int16_t s16;

struct Sample
{
    s16 left;
    s16 right;
};

struct AudioStream
{
    Sample* samples;
    int size;
    int pos;
};

void audio_callback(void* _userdata, unsigned char* dst, int byte_size);

void sdl_error()
{
    printf("SDL2 error: %s\n", SDL_GetError());
    exit(1);
}

// todo: 'culled' frequencies seem not to be 100% culled, I don't know why (it is not that bad visually)
// + there is some weird sound distortion happening when culling is performed

class Scene: public hppv::Prototype
{
public:
    // member variables are as bad as the global ones..., unless member functions are very small which is not the case here
    AudioStream stream;
    Complex* fft_buf;
    Complex* fft_buf2;
    Sample* audio_buf_unmod;
    Sample* audio_buf;
    Sample* next_audio_buf;
    std::atomic_bool prepare_next;
    SDL_AudioDeviceID devid;
    bool pause_on_click = true;
    bool show_unmod = false;
    glm::vec2 cursor_click_pos;
    glm::vec2 cursor_current_pos;
    bool lmb = false;
    int cull_fq_beg = 1;
    int cull_fq_end = 1;

    Scene(): hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {
        stream.pos = 0;
        prepare_next = false;
        fft_buf = (Complex*)malloc(sizeof(Complex) * SAMPLE_COUNT);
        fft_buf2 = (Complex*)malloc(sizeof(Complex) * SAMPLE_COUNT);
        audio_buf = (Sample*)malloc(sizeof(Sample) * SAMPLE_COUNT);
        next_audio_buf = (Sample*)malloc(sizeof(Sample) * SAMPLE_COUNT);
        memset(audio_buf, 0, sizeof(Sample) * SAMPLE_COUNT);
        memset(next_audio_buf, 0, sizeof(Sample) * SAMPLE_COUNT);

        if(SDL_Init(SDL_INIT_AUDIO))
            sdl_error();

        SDL_AudioSpec spec;
        unsigned char* tmp_buf;
        unsigned int tmp_size;

        if(!SDL_LoadWAV("/home/mat/output.wav", &spec, &tmp_buf, &tmp_size))
            sdl_error();

        assert(spec.channels == 2);
        assert(spec.freq == AUDIO_FQ);
        assert(spec.samples == SAMPLE_COUNT);
        assert(spec.format == AUDIO_S16LSB);

        stream.samples = (Sample*)tmp_buf;
        stream.size = tmp_size / sizeof(Sample);
        assert(stream.size > SAMPLE_COUNT);
        audio_buf_unmod = stream.samples;

        spec.userdata = this;
        spec.callback = audio_callback;
        SDL_AudioSpec spec_used;
        devid = SDL_OpenAudioDevice(nullptr, false, &spec, &spec_used, 0);

        if(!devid)
            sdl_error();

        SDL_PauseAudioDevice(devid, 0);
    }

    ~Scene()
    {
        SDL_Quit();
    }

    void prototypeProcessInput(hppv::Pinput in) override
    {
        cursor_current_pos = in.cursorPos;

        if(in.lmb && !lmb)
        {
            lmb = true;
            cursor_click_pos = in.cursorPos;
        }
        else if(!in.lmb && lmb)
        {
            lmb = false;

            if(in.cursorPos != cursor_click_pos)
            {
                // cursor to world coords
                float x_beg = hppv::mapCursor(cursor_click_pos, space_.projected, this).x;
                float x_end = hppv::mapCursor(in.cursorPos, space_.projected, this).x;
                // world coords to a frequency axis
                x_beg = map(x_beg, 0.f, 5.f, 0.f, 100.f);
                x_end = map(x_end, 0.f, 5.f, 0.f, 100.f);

                if(x_beg > x_end)
                {
                    float tmp = x_beg;
                    x_beg = x_end;
                    x_end = tmp;
                }
                if(x_beg < 0.f)
                    x_beg = 0.f;
                if(x_end < 0.f)
                    x_end = 0.f;
                if(x_beg > 5.f)
                    x_beg = 5.f;
                if(x_end > 5.f)
                    x_end = 5.f;

                float real_fq_beg = powf(10.f, x_beg);
                float real_fq_end = powf(10.f, x_end);
                int fq_beg = real_fq_beg * SAMPLE_COUNT / AUDIO_FQ;
                int fq_end = real_fq_end * SAMPLE_COUNT / AUDIO_FQ;
                int nyquist = SAMPLE_COUNT / 2;

                if(fq_beg < 1)
                    fq_beg = 1;
                if(fq_end < 1)
                    fq_end = 1;
                if(fq_beg > nyquist)
                    fq_beg = nyquist;
                if(fq_end > nyquist)
                    fq_end = nyquist;

                cull_fq_beg = fq_beg;
                cull_fq_end = fq_end;
            }
            else
            {
                cull_fq_beg = 1;
                cull_fq_end = 1;
            }
        }
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        if(prepare_next)
        {
            audio_buf_unmod = stream.samples + stream.pos - SAMPLE_COUNT;
            memcpy(audio_buf, next_audio_buf, sizeof(Sample) * SAMPLE_COUNT); // next buffer is now the current buffer

            if(stream.pos + SAMPLE_COUNT > stream.size)
                stream.pos = 0;

            // fill fft buffer for each channel
            for(int i = 0; i < SAMPLE_COUNT; ++i)
            {
                Sample sample = stream.samples[stream.pos + i];
                fft_buf[i] =  {(float)sample.left  / ((1 << 15) - 1), 0.f};
                fft_buf2[i] = {(float)sample.right / ((1 << 15) - 1), 0.f};
                // todo, window function?
            }

            fft(fft_buf, SAMPLE_COUNT);
            fft(fft_buf2, SAMPLE_COUNT);

            // silence target frequencies

            assert(cull_fq_beg > 0); // don't cull a DC component, it is not even displayed on a screen
            assert(cull_fq_beg <= cull_fq_end);
            assert(cull_fq_end <= SAMPLE_COUNT / 2);

            // frequency culling; todo, I'm not sure if this code is correct

            for(int k = cull_fq_beg; k < cull_fq_end; ++k)
                fft_buf[k] = fft_buf2[k] = fft_buf[SAMPLE_COUNT - k] = fft_buf2[SAMPLE_COUNT - k] = {0.f, 0.f};

            // perform inverse FFT; for some explanation see RealDFT example

            for(int k = 0; k < SAMPLE_COUNT; ++k)
            {
                fft_buf[k].im *= -1;
                fft_buf2[k].im *= -1;
            }

            fft(fft_buf, SAMPLE_COUNT);
            fft(fft_buf2, SAMPLE_COUNT);

            // note, in practice this should never block; main thread has ~90 ms to update next_audio_buf before audio callback
            // executes again (given 4k samples and 44k fq)
            SDL_LockAudio();

            // use this instead to play an unmodified stream
            //memcpy(next_audio_buf, stream.samples + stream.pos, sizeof(Sample) * SAMPLE_COUNT);

            for(int i = 0; i < SAMPLE_COUNT; ++i)
            {
                float amp_left = fft_buf[i].re / SAMPLE_COUNT;
                float amp_right = fft_buf2[i].re / SAMPLE_COUNT;
                // todo: these asserts are sometimes triggered when culling is enabled
                //assert(fabs(amp_left) <= 1.f);
                //assert(fabs(amp_right) <= 1.f);
                // workaround
                if(amp_left > 1.f)
                    amp_left = 1.f;
                if(amp_right > 1.f)
                    amp_right = 1.f;

                next_audio_buf[i].left = amp_left * ((1 << 15) - 1);
                next_audio_buf[i].right = amp_right * ((1 << 15) - 1);
            }
            prepare_next = false;
            SDL_UnlockAudio();
            stream.pos += SAMPLE_COUNT;
        }

        ImGui::Begin(prototype_.imguiWindowName);

        if(ImGui::Button("play / pause"))
        {
            SDL_PauseAudioDevice(devid, pause_on_click);
            pause_on_click = !pause_on_click;
        }
        ImGui::Checkbox("show unmodified audio", &show_unmod);
        ImGui::End();

        // fill fft buffer

        for(int i = 0; i < SAMPLE_COUNT; ++i)
        {
            Sample sample = show_unmod ? audio_buf_unmod[i] : audio_buf[i];
            float amp = sample.left + sample.right;
            // normalize
            amp /= 2.f;
            amp /= (1 << 15) - 1;
            // apply Hamming window; todo: understand more about window functions; at least it looks good on amplitude / time graph;
            // but it looks bad on the 1st frequency when it is culled; commented out for now
            //int N = SAMPLE_COUNT;
            //amp *= 0.54f + 0.46f * cosf(2.f * pi * (i - N / 2.f) / N);
            fft_buf[i] = {amp, 0};
        }

        rr.mode(hppv::RenderMode::Vertices);
        rr.shader(hppv::Render::VerticesColor);

        rr.primitive(GL_POINTS);

        // render amplitude over time

        for(int n = 0; n < SAMPLE_COUNT; ++n)
        {
            hppv::Vertex v;
            v.color = {0.4f, 0.1f, 0.1f, 0.1f};
            v.pos.x = map(n, 30.f, 70.f, 0, SAMPLE_COUNT);
            // todo: I don't know, I will apply a window function here.
            int N = SAMPLE_COUNT;
            v.pos.y = 20.f + -10.f * fft_buf[n].re * (0.54f + 0.46f * cosf(2.f * pi * (n - N / 2.f) / N));
            rr.cache(&v, 1);
        }

        fft(fft_buf, SAMPLE_COUNT);

        rr.primitive(GL_LINE_STRIP);

        // render a frequency spectrum

        for(int k = 1; k < SAMPLE_COUNT / 2; ++k) // skip a DC component and frequencies above the Nyquist fq
        {
            Complex c = fft_buf[k];
            float amp = sqrtf(c.re * c.re + c.im * c.im) * 2.f / SAMPLE_COUNT;
            float real_fq = (float)(k * AUDIO_FQ) / SAMPLE_COUNT;
            float x = log10f(real_fq);
            x = map(x, 0.f, 100.f, 0.f, 5.f);
            hppv::Vertex v;
            v.pos.x = x;
            v.pos.y = 80.f + -120.f * amp;
            rr.cache(&v, 1);
        }

        // render a culled frequency band
        rr.breakBatch();
        {
            float x_beg;
            float x_end;

            if(!lmb)
            {
                float real_fq_beg = (float)(cull_fq_beg * AUDIO_FQ) / SAMPLE_COUNT ;
                float real_fq_end = (float)(cull_fq_end * AUDIO_FQ) / SAMPLE_COUNT ;
                x_beg = map(log10f(real_fq_beg), 0.f, 100.f, 0.f, 5.f);
                x_end = map(log10f(real_fq_end), 0.f, 100.f, 0.f, 5.f);
            }
            else
            {
                x_beg = hppv::mapCursor(cursor_click_pos, space_.projected, this).x;
                x_end = hppv::mapCursor(cursor_current_pos, space_.projected, this).x;
            }
            hppv::Vertex v;
            v.color = {1.f, 0.f, 0.f, 1.f};
            v.pos.y = 82.f;
            v.pos.x = x_beg;
            rr.cache(&v, 1);
            v.pos.x = x_end;
            rr.cache(&v, 1);
        }
    }
};

void audio_callback(void* _userdata, unsigned char* dst, int byte_size)
{
    assert(byte_size / sizeof(Sample) == SAMPLE_COUNT);
    Scene* scene = (Scene*)_userdata;
    memcpy(dst, scene->next_audio_buf, byte_size);
    scene->prepare_next = true;
}

RUN(Scene)
