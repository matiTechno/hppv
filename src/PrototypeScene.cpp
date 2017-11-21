#include <string>

#include <GLFW/glfw3.h>

#include <hppv/PrototypeScene.hpp>
#include <hppv/App.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

namespace hppv
{

PrototypeScene::PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor):
    space_(space),
    zoomFactor_(zoomFactor),
    alwaysZoomToCursor_(alwaysZoomToCursor)
{
    properties_.maximize = true;
    rmb_.pressed = false;

    rmb_.pos = frame_.cursorPos;
}

void PrototypeScene::processInput(bool hasInput)
{
    space_.newFrame(properties_.size);

    for(auto& event: frame_.events)
    {
        if(event.type == Event::Cursor)
        {
            if(rmb_.pressed && hasInput)
            {
                auto newSpaceCoords = mapCursor(event.cursor.pos, space_.projected, this);
                auto prevSpaceCoords = mapCursor(rmb_.pos, space_.projected, this);
                space_.set({space_.current.pos - (newSpaceCoords - prevSpaceCoords), space_.current.size});
            }

            rmb_.pos = event.cursor.pos;
        }
        else if(event.type == Event::MouseButton && event.mouseButton.button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            rmb_.pressed = event.mouseButton.action == GLFW_PRESS;
        }
        else if(event.type == Event::Scroll && hasInput)
        {
            auto zoom = glm::pow(zoomFactor_, event.scroll.offset.y);

            if(rmb_.pressed || alwaysZoomToCursor_)
            {
                auto spaceCoords = mapCursor(rmb_.pos, space_.projected, this);
                space_.set(zoomToPoint(space_.current, zoom, spaceCoords));
            }
            else
            {
                space_.set(zoomToCenter(space_.current, zoom));
            }
        }
        else if(event.type == Event::FramebufferSize)
        {
            rmb_.pos = frame_.cursorPos;
        }
    }

    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    renderer.projection(space_.projected);
    prototypeRender(renderer);

    if(!renderImGui_)
        return;

    ++frameCount_;
    accumulator_ += frame_.frameTime;

    if(accumulator_ >= 1.f)
    {
        auto averageFrameTime = accumulator_ / frameCount_;
        averageFrameTimeMs_ = averageFrameTime * 1000.f;
        averageFps_ = 1.f / averageFrameTime + 0.5f;
        frameCount_ = 0;
        accumulator_ = 0.f;
    }

    ImGui::Begin("PrototypeScene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by            m a t i T e c h n o");
        ImGui::Text("frameTime          %f ms", averageFrameTimeMs_);
        ImGui::Text("fps                %d",    averageFps_);
        ImGui::Text("framebuffer size   %d, %d", frame_.framebufferSize.x, frame_.framebufferSize.y);

        if(ImGui::Checkbox("vsync", &vsync_))
        {
            Request request(Request::Vsync);
            request.vsync.on = vsync_;
            App::request(request);
        }

        // window state
        if(frame_.window.state == Frame::Window::Fullscreen)
        {
            if(ImGui::Button("restore"))
            {
                Request request(Request::Window);
                request.window.state = Frame::Window::Restored;
                App::request(request);
            }
        }
        else
        {
            if(ImGui::Button("set fullscreen"))
            {
                Request request(Request::Window);
                request.window.state = Frame::Window::Fullscreen;
                App::request(request);
            }

            ImGui::SameLine();

            if(frame_.window.state == Frame::Window::Maximized)
            {
                if(ImGui::Button("restore"))
                {
                    Request request(Request::Window);
                    request.window.state = Frame::Window::Restored;
                    App::request(request);
                }
            }
            else
            {
                if(ImGui::Button("maximize"))
                {
                    Request request(Request::Window);
                    request.window.state = Frame::Window::Maximized;
                    App::request(request);
                }
            }
        }

        if(ImGui::Button("quit"))
        {
            App::request(Request::Quit);
        }

        ImGui::Separator();
        ImGui::Text("rmb      move around");

        std::string zoomInfo("scroll   zoom to ");

        if(alwaysZoomToCursor_)
        {
            zoomInfo += "cursor";
        }
        else
        {
            zoomInfo += "center / cursor if rmb";
        }

        ImGui::Text("%s", zoomInfo.c_str());

        ImGui::Separator();
        auto spaceCoords = mapCursor(rmb_.pos, space_.projected, this);
        ImGui::Text("space coords       %.3f, %.3f", spaceCoords.x, spaceCoords.y);
    }
    ImGui::End();
}

} // namespace hppv
