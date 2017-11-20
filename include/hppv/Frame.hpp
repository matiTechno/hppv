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
    // todo: make it optional; if window does not have focus set it empty
    glm::vec2 cursorPos;
    glm::ivec2 framebufferSize;

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
    }
    window;
};

} // namespace hppv
