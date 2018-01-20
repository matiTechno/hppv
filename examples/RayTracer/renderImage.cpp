#include "renderImage.hpp"

void renderImage(Pixel* buffer, const glm::ivec2 size, std::atomic_int& progress)
{
    for(auto j = 0; j < size.y; ++j)
    {
        for(auto i = 0; i < size.x; ++i)
        {
            buffer->r = static_cast<float>(i + 1) / size.x * 255;
            ++buffer;
            ++progress;
        }
    }
}
