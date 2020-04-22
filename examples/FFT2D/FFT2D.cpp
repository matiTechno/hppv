#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <vector>
#include <stdlib.h>

#include <hppv/Prototype.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>

#include "../run.hpp"

#define pi (3.14159265359f)

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

// first with thrid and second with fourth; in matlab it is called fftshift

void swap_quadrants(Complex* cbuf, int width, int height)
{
    for(int y = 0; y < height / 2; ++y)
    {
        for(int x = 0; x < width / 2; ++x)
        {
            int idx1 = y * width + x;
            int idx2 = (y + height / 2) * width + x + width / 2;
            Complex tmp = cbuf[idx1];
            cbuf[idx1] = cbuf[idx2];
            cbuf[idx2] = tmp;
        }
    }

    for(int y = 0; y < height / 2; ++y)
    {
        for(int x = width / 2; x < width; ++x)
        {
            int idx1 = y * width + x;
            int idx2 = (y + height / 2) * width + x - width / 2;
            Complex tmp = cbuf[idx1];
            cbuf[idx1] = cbuf[idx2];
            cbuf[idx2] = tmp;
        }
    }
}

struct Textures
{
    hppv::Texture spectrum;
    hppv::Texture filtered;
    float aspect;
};

typedef unsigned char u8;

#define GEN_LOWPASS 1
#define GEN_HIGHPASS 0

// think of the filter as of a square, cutoff_fq itself is not filtered out
// the DC component is always preserved

Textures gen_textures(const u8* image, int width, int height, int cutoff_fq, bool lowpass)
{
    Complex* cbuf = (Complex*)malloc(width * height * sizeof(Complex));

    // move data to a format required by FFT function

    for(int i = 0; i < width * height; ++i)
        cbuf[i] = {image[i * 4] / 255.f, 0.f};

    // first, compute FFT for each row of an image

    for(int y = 0; y < height; ++y)
        fft(cbuf + y * width, width);

    // compute FFT for each column based on the previous results
    // we need to temporarily relocate a column to make it continuous in a memory

    Complex* column = (Complex*)malloc(height * sizeof(Complex));

    for(int x = 0; x < width; ++x)
    {
        for(int y = 0; y < height; ++y)
            column[y] = cbuf[y * width + x];

        fft(column, height);

        // copy the result to a main buffer

        for(int y = 0; y < height; ++y)
            cbuf[y * width + x] = column[y];
    }

    // this helps with visualization and filtering; quadrants positions are restored before performing inverse FFT

    swap_quadrants(cbuf, width, height);

    // apply a filter

    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            int dist_x = abs(x - (width / 2));
            int dist_y = abs(y - (height / 2));
            int max_dist = dist_x > dist_y ? dist_x : dist_y;

            if(lowpass)
            {
                if(max_dist > cutoff_fq)
                    cbuf[y * width + x] = {0.f, 0.f};
            }
            else
            {
                if(max_dist != 0 && max_dist < cutoff_fq)
                    cbuf[y * width + x] = {0.f, 0.f};
            }
        }
    }

    // create a spectrum texture

    float max = 0.f;

    for(int i = 0; i < width * height; ++i)
    {
        Complex c = cbuf[i];
        float mod = sqrtf(c.re * c.re + c.im * c.im);
        float v = logf(mod);

        if(v > max)
            max = v;
    }

    u8* tmp_image = (u8*)malloc(width * height * 4);

    for(int i = 0; i < width * height; ++i)
    {
        Complex c = cbuf[i];
        float mod = sqrtf(c.re * c.re + c.im * c.im);
        float v = logf(mod) / max;

        if(v < 0.f)
            v = 0.f;

        assert(v <= 1.f);

        for(int k = 0; k < 4; ++k)
            tmp_image[i * 4 + k] = v * 255.f + 0.5f;
    }

    hppv::Texture spectrum_tex(GL_RGBA8, {width, height});
    spectrum_tex.bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp_image);

    // undo swapping

    swap_quadrants(cbuf, width, height);

    // reconstruct the image using iFFT

    for(int i = 0; i < width * height; ++i)
        cbuf[i].im *= -1.f; // see RealDFT.cpp for some explanation

    // rows

    for(int y = 0; y < height; ++y)
        fft(cbuf + y * width, width);

    // columns

    for(int x = 0; x < width; ++x)
    {
        for(int y = 0; y < height; ++y)
            column[y] = cbuf[y * width + x];

        fft(column, height);

        for(int y = 0; y < height; ++y)
            cbuf[y * width + x] = column[y];
    }

    // divide by the number of samples

    for(int i = 0; i < width * height; ++i)
        cbuf[i].re /= width * height;

    // create a texture of a filtered image

    for(int i = 0; i < width * height; ++i)
    {
        int v = cbuf[i].re * 255.f + 0.5f;

        if(v > 255)
            v = 255;

        for(int k = 0; k < 4; ++k)
            tmp_image[i * 4 + k] = v;
    }

    hppv::Texture filtered_tex(GL_RGBA8, {width, height});
    filtered_tex.bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tmp_image);

    free(cbuf);
    free(column);
    free(tmp_image);
    return {std::move(spectrum_tex), std::move(filtered_tex), (float)width / height};
}

int adjust_to_pow2(int n)
{
    assert(n);

    if((n & (n - 1)) == 0)
        return n;

    int k = 0;

    while(n > 0)
    {
        ++k;
        n /= 2;
    }
    return 1 << k;
}

void load_pgm(const char* filename, u8*& buf, int& width, int& height)
{
    FILE* file = fopen(filename, "rb");
    assert(file);
    int load_width, load_height;
    int n = fscanf(file, "P5 %d %d 255", &load_width, &load_height);
    assert(n == 2);
    width = adjust_to_pow2(load_width);
    height = adjust_to_pow2(load_height);
    fgetc(file);
    buf = (u8*)malloc(width * height * 4); // 4 channels per pixel, I don't want to write a special shader to handle grayscale textures
    memset(buf, 30, width * height * 4);

    for(int y = 0; y < load_height; ++y)
    {
        for(int x = 0; x < load_width; ++x)
        {
            unsigned char tmp;
            int n = fread(&tmp, 1, 1, file);
            assert(n == 1);
            int px_idx = y * width + x;

            for(int k = 0; k < 4; ++k)
                buf[px_idx * 4 + k] = tmp;
        }
    }
    fclose(file);
}

hppv::Texture black_dummy_texture()
{
    hppv::Texture tex(GL_RGBA8, {1, 1});
    tex.bind();
    u8 buf[] = {0, 0, 0, 0};
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    return std::move(tex);
}

class FFT2D: public hppv::Prototype
{
public:
    std::vector<Textures> textures;

    FFT2D(): hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {
        u8* buf;
        int width;
        int height;
        hppv::Texture unmod_tex;

        load_pgm("res/lenna.pgm", buf, width, height);
        unmod_tex = hppv::Texture(GL_RGBA8, {width, height});
        unmod_tex.bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        textures.push_back({black_dummy_texture(), std::move(unmod_tex), (float)width / height});
        textures.push_back(gen_textures(buf, width, height, 0, GEN_HIGHPASS));
        textures.push_back(gen_textures(buf, width, height, 50, GEN_LOWPASS));
        textures.push_back(gen_textures(buf, width, height, 20, GEN_HIGHPASS));

        load_pgm("res/barbara.pgm", buf, width, height);
        unmod_tex = hppv::Texture(GL_RGBA8, {width, height});
        unmod_tex.bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        textures.push_back({black_dummy_texture(), std::move(unmod_tex), (float)width / height});
        textures.push_back(gen_textures(buf, width, height, 0, GEN_HIGHPASS));
        textures.push_back(gen_textures(buf, width, height, 30, GEN_LOWPASS));
        textures.push_back(gen_textures(buf, width, height, 40, GEN_HIGHPASS));
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        rr.flipTextureY(true);
        rr.shader(hppv::Render::Tex);
        hppv::Sprite s;
        s.pos = {0.f, 0.f};

        for(Textures& tex: textures)
        {
            s.size = {30.f, 30.f};
            s.size.x *= tex.aspect;
            rr.sampler(hppv::Sample::Linear);
            rr.texture(tex.filtered);
            rr.cache(s);
            s.pos.y += 35.f;
            rr.sampler(hppv::Sample::Nearest);
            rr.texture(tex.spectrum);
            rr.cache(s);
            s.pos.y -= 35.f;
            s.pos.x += 35.f * tex.aspect;
        }
    }
};

RUN(FFT2D)
