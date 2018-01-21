#pragma once

#include <atomic>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct Pixel
{
    Pixel() = default;
    unsigned char r = 0, g = 0, b = 0;
};

// progress - incremented for each pixel rendered

void renderImage(Pixel* buffer, glm::ivec2 size, std::atomic_int& progress);
