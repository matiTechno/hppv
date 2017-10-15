#include "Space.hpp"
#include "Scene.hpp"

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

Space zoomToCursor(Space space, float zoom, glm::vec2 cursorPos, const Scene& scene,
                   glm::ivec2 framebufferSize)
{
    auto pos    = cursorSpacePos(space, cursorPos, scene, framebufferSize);
    space.size *= 1.f / zoom;
    auto newPos = cursorSpacePos(space, cursorPos, scene, framebufferSize);
    space.pos -= newPos - pos;
    return space;
}

glm::vec2 cursorSpacePos(Space space, glm::vec2 cursorPos, const Scene& scene,
                         glm::ivec2 framebufferSize)
{
    cursorPos = cursorScenePos(scene, cursorPos, framebufferSize);

    return space.pos + (space.size / glm::vec2(scene.properties_.size))
                       * (cursorPos - glm::vec2(scene.properties_.pos));
}

glm::vec2 cursorScenePos(const Scene& scene, glm::vec2 cursorPos,
                         glm::ivec2 framebufferSize)
{
    return glm::vec2(scene.properties_.pos) + (glm::vec2(scene.properties_.size)
                                              / glm::vec2(framebufferSize))
                                              * cursorPos;
}
