#pragma once

#include <glm/vec2.hpp>

namespace hppv
{

struct Event
{
    enum Type
    {
        Quit,
        FocusLost,
        FocusGained,
        FramebufferSize,
        Key,
        Cursor,
        MouseButton,
        Scroll
    };

    Event() = default;
    Event(Type type): type(type) {}

    Type type;

    union
    {
        struct
        {
            glm::ivec2 prevSize;
            glm::ivec2 newSize;
        }
        framebufferSize;

        struct
        {
            int key;
            int action;
            int mods;
        }
        key;

        struct
        {
            glm::vec2 pos;
        }
        cursor;

        struct
        {
            int button;
            int action;
            int mods;
        }
        mouseButton;

        struct
        {
            glm::vec2 offset;
        }
        scroll;
    };
};

} // namespace hppv
