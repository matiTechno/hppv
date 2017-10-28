#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "Event.hpp"

namespace hppv
{

struct Frame
{
    float frameTime;
    glm::ivec2 framebufferSize; // initialized in App::initialize()
    std::vector<Event> events;
};

} // namespace hppv
