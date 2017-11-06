#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "Event.hpp"

namespace hppv
{

struct Frame
{
    float frameTime;

    // initialized in App::initialize()
    glm::ivec2 framebufferSize;
    bool fullscreen;

    std::vector<Event> events;

    // internal use
    struct
    {
        glm::ivec2 pos;
        glm::ivec2 size;
    }
    windowedState;
};

} // namespace hppv
