#pragma once

#include <glm/vec2.hpp>

namespace hppv
{

class Scene;

struct Rect
{
    Rect() = default;
    Rect(float x, float y, float w, float h): pos(x, y), size(w, h) {}
    Rect(glm::vec2 pos, glm::vec2 size): pos(pos), size(size) {}
    Rect(glm::vec2 pos, float w, float h): pos(pos), size(w, h) {}
    Rect(float x, float y, glm::vec2 size): pos(x, y), size(size) {}

    glm::vec2 pos;
    glm::vec2 size;
};

// inspired by glm matrix transform functions
// return new object instead of modifying existing one

Rect expandToMatchAspectRatio(Rect rect, glm::ivec2 size);

Rect zoomToCenter(Rect rect, float zoom);

Rect zoomToPoint(Rect space, float zoom, glm::vec2 point);

glm::vec2 mapCursorToRect(Rect rect, glm::vec2 cursorPos, const Scene& scene);

Rect mapToScene(Rect rect, Rect projection, const Scene& scene);

Rect mapToWindow(Rect rect, Rect projection, const Scene& scene);

} // namespace hppv
