#include <limits>

#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Framebuffer.hpp>

namespace hppv
{

static_assert(std::numeric_limits<float>::is_iec559);

Space expandToMatchAspectRatio(const Space space, const glm::ivec2 size)
{
    const auto spaceAspect = space.size.x / space.size.y;
    const auto targetAspect = static_cast<float>(size.x) / size.y;
    auto newSize = space.size;

    if(spaceAspect < targetAspect)
    {
        newSize.x = targetAspect * newSize.y;
    }
    else if(spaceAspect > targetAspect)
    {
        newSize.y = newSize.x / targetAspect;
    }

    const auto newPos = space.pos - (newSize - space.size) / 2.f;
    return {newPos, newSize};
}

Space zoomToPoint(Space space, const float zoom, const glm::vec2 point)
{
    const auto ratio = (point - space.pos) / space.size;
    space.size *= 1.f / zoom;
    const auto newRatio = (point - space.pos) / space.size;
    space.pos -= (ratio - newRatio) * space.size;
    return space;
}

glm::vec2 mapCursor(glm::vec2 pos, const Space projection, const Scene* const scene)
{
    pos -= glm::vec2(scene->properties_.pos);
    return map(pos, {0, 0, scene->properties_.size}, projection);
}

glm::vec4 mapToFramebuffer(const glm::vec4 rect, const Space projection, const Framebuffer& fb)
{
    return map(rect, projection, {0, 0, fb.getSize()});
}

glm::vec4 mapToScene(const glm::vec4 rect, const Space projection, const Scene* const scene)
{
    return map(rect, projection, {0, 0, scene->properties_.size});
}

glm::ivec4 iMapToWindow(const glm::vec4 rect, const Space projection, const Scene* const scene)
{
    auto mapped = iMapToScene(rect, projection, scene);
    mapped.x += scene->properties_.pos.x;
    mapped.y += scene->properties_.pos.y;
    return mapped;
}

} // namespace hppv
