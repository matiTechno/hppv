#include "Space.hpp"

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
