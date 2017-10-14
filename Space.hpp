#pragma once

#include <glm/vec2.hpp>

struct Space
{
    Space() = default;
    Space(float x, float y, float w, float h):
        pos(x, y),
        size(w, h)
    {}

    Space(glm::vec2 pos, glm::vec2 size):
        pos(pos),
        size(size)
    {}

    glm::vec2 pos;
    glm::vec2 size;

};

Space createExpandedToMatchAspectRatio(Space space, glm::ivec2 size);
