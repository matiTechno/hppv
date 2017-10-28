#pragma once

#include <glm/vec2.hpp>

namespace hppv
{

class Scene;

struct Space
{
    Space() = default;
    Space(float x, float y, float w, float h): pos(x, y), size(w, h) {}
    Space(glm::vec2 pos, glm::vec2 size): pos(pos), size(size) {}
    Space(glm::vec2 pos, float w, float h): pos(pos), size(w, h) {}
    Space(float x, float y, glm::vec2 size): pos(x, y), size(size) {}

    glm::vec2 pos;
    glm::vec2 size;
};

// inspired by glm matrix transform functions
// return new object instead of modifying existing one

Space expandToMatchAspectRatio(Space space, glm::ivec2 size);

Space zoomToCenter(Space space, float zoom);

Space zoomToPoint(Space space, float zoom, glm::vec2 point);

glm::vec2 mapToProjection(glm::vec2 cursorPos, Space projection, const Scene& scene);

glm::ivec4 mapToWindow(glm::vec4 rect, Space projection, const Scene& scene);

glm::ivec4 mapToScene(glm::vec4 rect, Space projection, const Scene& scene);

} // namespace hppv
