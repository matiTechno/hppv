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

    Event(Type type): type(type) {}
    Event() = default;

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
