#pragma once

#include <glm/vec2.hpp>
class Scene;

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

// inspired by glm matrix transform functions
// return new object instead of modifying existing one

Space expandToMatchAspectRatio(Space space, glm::ivec2 size);

Space zoomToCenter(Space space, float zoom);

Space zoomToPoint( Space space, float zoom, glm::vec2 point);

Space zoomToCursor(Space space, float zoom, glm::vec2 cursorPos, const Scene& scene);

glm::vec2 cursorSpacePos(Space space, glm::vec2 cursorPos, const Scene& scene);
