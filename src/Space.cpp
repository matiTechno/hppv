#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>

namespace hppv
{

Space::Space(float x, float y, float w, float h):
    pos(x, y),
    size(w, h)
{}

Space::Space(glm::vec2 pos, glm::vec2 size):
    pos(pos),
    size(size)
{}

Space expandToMatchAspectRatio(Space space, glm::ivec2 size)
{
    auto spaceAspect = space.size.x / space.size.y;

    auto targetAspect = static_cast<float>(size.x) / size.y;

    auto newSize = space.size;

    if(spaceAspect < targetAspect)
        newSize.x = targetAspect * newSize.y;

    else if(spaceAspect > targetAspect)
        newSize.y = newSize.x / targetAspect;

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

glm::vec2 cursorSpacePos(Space space, glm::vec2 cursorPos, const Scene& scene)
{
    cursorPos -= glm::vec2(scene.properties_.pos);
    return space.pos + (space.size / glm::vec2(scene.properties_.size))
                       * cursorPos;
}

glm::ivec4 spaceToWindow(glm::ivec4 rect, Space projection, const Scene& scene)
{

}

} // namespace hppv
