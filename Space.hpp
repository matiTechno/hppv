#pragma once

#include <glm/vec2.hpp>

class Space
{
public:
    Space() = default;
    Space(float x, float y, float w, float h):
        pos_(x, y),
        size_(w, h)
    {}

    Space(glm::vec2 pos, glm::vec2 size):
        pos_(pos),
        size_(size)
    {}

    glm::vec2 pos_;
    glm::vec2 size_;

};

Space createExpandedToMatchAspectRatio(Space space, glm::ivec2 size);
