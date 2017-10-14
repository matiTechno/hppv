#include "Space.hpp"

Space createExpandedToMatchAspectRatio(Space space, glm::ivec2 size)
{
    auto spaceAspect = space.size_.x / space.size_.y;

    auto targetAspect = static_cast<float>(size.x) / size.y;

    auto newSize = space.size_;

    if(spaceAspect < targetAspect)
        newSize.x = targetAspect * newSize.y;
    
    else if(spaceAspect > targetAspect)
        newSize.y = newSize.x / targetAspect;
    
    auto newPos = space.pos_ - (newSize - space.size_) / 2.f;

    return {newPos, newSize};
}
