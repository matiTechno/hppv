#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "Event.hpp"

namespace hppv
{

// initialized in App::initialize()
struct Frame
{
    float frameTime;
    std::vector<Event> events;
    glm::vec2 cursorPos;
    glm::ivec2 framebufferSize;

    struct Window
    {
        enum State
        {
            Restored,
            Fullscreen,
            Maximized
        }
        state;

        struct
        {
            glm::ivec2 pos;
            glm::ivec2 size;
        }
        restored;
    }
    window;
};

} // namespace hppv
