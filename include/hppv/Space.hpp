#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace hppv
{

class Scene;
class Framebuffer;

struct Space
{
    Space() = default;
    Space(float x, float y, float w, float h): pos(x, y), size(w, h) {}
    Space(glm::vec2 pos, glm::vec2 size): pos(pos), size(size) {}
    Space(glm::vec2 pos, float w, float h): pos(pos), size(w, h) {}
    Space(float x, float y, glm::vec2 size): pos(x, y), size(size) {}
    Space(glm::vec4 vec): pos(vec.x, vec.y), size(vec.z, vec.w) {}

    glm::vec2 pos;
    glm::vec2 size;
};

Space expandToMatchAspectRatio(Space space, glm::ivec2 size);

// -----

Space zoomToCenter(Space space, float zoom);

// -----

Space zoomToPoint(Space space, float zoom, glm::vec2 point);

// -----

inline glm::vec2 map(glm::vec2 point, Space from, Space to)
{
    return to.pos + to.size / from.size * (point - from.pos);
}

// -----

inline glm::ivec2 iMap(glm::vec2 point, Space from, Space to)
{
    return map(point, from, to) + 0.5f;
}

// -----

inline glm::vec4 map(glm::vec4 rect, Space from, Space to)
{
    auto pos = map({rect.x, rect.y}, from, to);
    auto size = glm::vec2(rect.z, rect.w) * to.size / from.size;
    return {pos, size};
}

// -----

inline glm::ivec4 iMap(glm::vec4 rect, Space from, Space to)
{
    return map(rect, from, to) + 0.5f;
}

// -----

glm::vec2 mapCursor(glm::vec2 pos, Space projection, const Scene* scene);

// -----

glm::vec4 mapToFramebuffer(glm::vec4 rect, Space projection, const Framebuffer& fb);

// -----

glm::vec4 mapToScene(glm::vec4 rect, Space projection, const Scene* scene);

// -----

inline glm::ivec4 iMapToScene(glm::vec4 rect, Space projection, const Scene* scene)
{
    return mapToScene(rect, projection, scene) +  0.5f;
}

// -----

glm::ivec4 iMapToWindow(glm::vec4 rect, Space projection, const Scene* scene);

} // namespace hppv
