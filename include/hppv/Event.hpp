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
        Key,
        Cursor,
        MouseButton,
        Scroll,
        FramebufferSize
    }
    type;

    Event() = default;
    Event(Type type): type(type) {}

    union
    {
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
