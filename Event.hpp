#pragma once
#include <glm/vec2.hpp>

struct Event
{
    // window resize event should be handled by the proper use of Space class
    // and its modifier functions
    
    // as for now there is not need for cursor enter / leave events
    
    enum Type
    {
        Quit,
        FocusLost,
        FocusGained,
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
            int key;
            int action;
            int mods;
        } key;

        struct
        {
            glm::vec2 pos;
        } cursor;
        
        struct
        {
            int button;
            int action;
            int mods;
        } mouseButton;

        struct 
        {
            glm::vec2 offset;
        } scroll;
    };
};
