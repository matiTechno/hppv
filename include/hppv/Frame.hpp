#pragma once

#include <glm/vec2.hpp>

namespace hppv
{

struct Window
{
    enum State
    {
        Restored,
        Fullscreen,
        Maximized
    };

    State state;
    State previousState;

    struct
    {
        glm::ivec2 pos;
        glm::ivec2 size;
    }
    restored;
};

// initialized in App::initialize()
struct Frame
{
    float time; // in seconds
    glm::ivec2 framebufferSize;
    Window window;
};

} // namespace hppv
