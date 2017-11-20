#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Framebuffer.hpp>

namespace hppv
{

static_assert(std::numeric_limits<float>::is_iec559);

Space expandToMatchAspectRatio(Space space, glm::ivec2 size)
{
    auto spaceAspect = space.size.x / space.size.y;
    auto targetAspect = static_cast<float>(size.x) / size.y;
    auto newSize = space.size;

    if(spaceAspect < targetAspect)
    {
        newSize.x = targetAspect * newSize.y;
    }
    else if(spaceAspect > targetAspect)
    {
        newSize.y = newSize.x / targetAspect;
    }

    auto newPos = space.pos - (newSize - space.size) / 2.f;
    return {newPos, newSize};
}

Space zoomToCenter(Space space, float zoom)
{
    auto prevSize = space.size;
    space.size *= 1.f / zoom;
    space.pos += (prevSize - space.size) / 2.f;
    return space;
}

Space zoomToPoint(Space space, float zoom, glm::vec2 point)
{
    auto ratio = (point - space.pos) / space.size;
    space.size *= 1.f / zoom;
    auto newRatio = (point - space.pos) / space.size;
    space.pos -= (ratio - newRatio) * space.size;
    return space;
}

glm::vec2 mapCursor(glm::vec2 pos, Space projection, const Scene* scene)
{
    pos -= glm::vec2(scene->properties_.pos);
    return map(pos, {0.f, 0.f, scene->properties_.size}, projection);
}

glm::vec4 mapToFramebuffer(glm::vec4 rect, Space projection, const Framebuffer& fb)
{
    auto pos = map({rect.x, rect.y}, projection, {0.f, 0.f, fb.getSize()});
    auto size = glm::vec2(rect.z, rect.w) * (glm::vec2(fb.getSize()) / projection.size);
    return {pos, size};
}

glm::vec4 mapToScene(glm::vec4 rect, Space projection, const Scene* scene)
{
    auto pos = map({rect.x, rect.y}, projection, {0.f, 0.f, scene->properties_.size});
    auto size = glm::vec2(rect.z, rect.w) * (glm::vec2(scene->properties_.size) / projection.size);
    return {pos, size};
}

glm::ivec4 iMapToWindow(glm::vec4 rect, Space projection, const Scene* scene)
{
    auto mapped = iMapToScene(rect, projection, scene);
    mapped.x += scene->properties_.pos.x;
    mapped.y += scene->properties_.pos.y;
    return mapped;
}

} // namespace hppv
