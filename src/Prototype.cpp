#include <string>

#include <GLFW/glfw3.h>

#include <hppv/Prototype.hpp>
#include <hppv/App.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

namespace hppv
{

Prototype::Prototype(const Space space):
    space_(space)
{
    space_.newFrame(frame_.framebufferSize); // initialize space_.projected_
    properties_.maximize = true;
    cursorPos_ = App::getCursorPos();
}

void Prototype::processInput(const std::vector<Event>& events)
{
    space_.newFrame(properties_.size);

    for(const auto& event: events)
    {
        if(event.type == Event::Cursor)
        {
            if(rmb_)
            {
                const auto newSpaceCoords = mapCursor(event.cursor.pos, space_.projected, this);
                const auto prevSpaceCoords = mapCursor(cursorPos_, space_.projected, this);
                space_.set({space_.current.pos - (newSpaceCoords - prevSpaceCoords), space_.current.size});
            }

            cursorPos_ = event.cursor.pos;
        }
        else if(event.type == Event::MouseButton)
        {
            switch(event.mouseButton.button)
            {
            case GLFW_MOUSE_BUTTON_RIGHT: rmb_ = event.mouseButton.action == GLFW_PRESS; break;
            case GLFW_MOUSE_BUTTON_LEFT: lmb_ = event.mouseButton.action == GLFW_PRESS;
            }
        }
        else if(event.type == Event::Scroll)
        {
            const auto zoom = glm::pow(prototype_.zoomFactor, event.scroll.offset.y);

            if(rmb_ || lmb_ || prototype_.alwaysZoomToCursor)
            {
                const auto spaceCoords = mapCursor(cursorPos_, space_.projected, this);
                space_.set(zoomToPoint(space_.current, zoom, spaceCoords));
            }
            else
            {
                space_.set(zoomToCenter(space_.current, zoom));
            }
        }
        else if(event.type == Event::FramebufferSize)
        {
            cursorPos_ = App::getCursorPos();
        }
    }

    prototypeProcessInput({events, cursorPos_, lmb_});
}

void Prototype::render(Renderer& renderer)
{
    appWidget_.update(frame_);

    if(prototype_.renderImgui)
    {
        ImGui::Begin(prototype_.imguiWindowName);
        appWidget_.imgui(frame_);
        ImGui::Text("rmb      move around");

        std::string zoomInfo("scroll   zoom to ");

        if(prototype_.alwaysZoomToCursor)
        {
            zoomInfo += "cursor";
        }
        else
        {
            zoomInfo += "center /\n"
                        "                 cursor, if rmb or lmb";
        }

        ImGui::Text("%s", zoomInfo.c_str());
        const auto spaceCoords = mapCursor(cursorPos_, space_.projected, this);
        ImGui::NewLine();
        ImGui::Text("space coords   %.3f, %.3f", spaceCoords.x, spaceCoords.y);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::End();
    }

    renderer.projection(space_.projected);
    prototypeRender(renderer);
}

} // namespace hppv
