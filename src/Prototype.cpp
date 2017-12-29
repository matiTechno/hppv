#include <string>
#include <cstring>

#include <GLFW/glfw3.h>

#include <hppv/Prototype.hpp>
#include <hppv/App.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

namespace hppv
{

Prototype::Prototype(const Space space):
    space_(space),
    prototype_{lmb_, cursorPos_}
{
    properties_.maximize = true;
    cursorPos_ = frame_.cursorPos;

    std::memset(avgFrameTimesMs_, 0, sizeof(avgFrameTimesMs_));
}

void Prototype::processInput(const bool hasInput)
{
    space_.newFrame(properties_.size);

    for(const auto& event: frame_.events)
    {
        if(event.type == Event::Cursor)
        {
            if(rmb_ && hasInput)
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
        else if(event.type == Event::Scroll && hasInput)
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
            cursorPos_ = frame_.cursorPos;
        }
    }

    prototypeProcessInput(hasInput);
}

void Prototype::render(Renderer& renderer)
{
    ++frameCount_;
    accumulator_ += frame_.time;

    if(accumulator_ >= 0.0333f)
    {
        const auto avgFrameTime = accumulator_ / frameCount_;
        avgFrameTimeMs_ = avgFrameTime * 1000.f;
        avgFps_ = 1.f / avgFrameTime + 0.5f;
        frameCount_ = 0;
        accumulator_ = 0.f;

        std::memmove(avgFrameTimesMs_, avgFrameTimesMs_ + 1, sizeof(avgFrameTimesMs_) - sizeof(float));
        avgFrameTimesMs_[size(avgFrameTimesMs_) - 1] = avgFrameTimeMs_;
    }

    if(prototype_.renderImgui)
    {
        ImGui::Begin(prototype_.imguiWindowName);

        ImGui::Text("h p p v");
        ImGui::Text("frameTime          %f ms", avgFrameTimeMs_);
        ImGui::Text("fps                %d",    avgFps_);
        ImGui::Text("framebuffer size   %d, %d", frame_.framebufferSize.x, frame_.framebufferSize.y);

        if(ImGui::Checkbox("vsync", &vsync_))
        {
            Request request(Request::Vsync);
            request.vsync.on = vsync_;
            App::request(request);
        }

        // window state
        if(frame_.window.state == Window::Fullscreen)
        {
            if(ImGui::Button("restore"))
            {
                Request request(Request::Window);
                request.window.state = Window::Restored;
                App::request(request);
            }
        }
        else
        {
            if(ImGui::Button("set fullscreen"))
            {
                Request request(Request::Window);
                request.window.state = Window::Fullscreen;
                App::request(request);
            }

            ImGui::SameLine();

            if(frame_.window.state == Window::Maximized)
            {
                if(ImGui::Button("restore"))
                {
                    Request request(Request::Window);
                    request.window.state = Window::Restored;
                    App::request(request);
                }
            }
            else
            {
                if(ImGui::Button("maximize"))
                {
                    Request request(Request::Window);
                    request.window.state = Window::Maximized;
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

        ImGui::Separator();
        const auto spaceCoords = mapCursor(cursorPos_, space_.projected, this);
        ImGui::Text("space coords       %.3f, %.3f", spaceCoords.x, spaceCoords.y);

        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_PlotLines, {1.f, 1.f, 0.f, 1.f});
        ImGui::PlotLines("frameTime(ms)", avgFrameTimesMs_, size(avgFrameTimesMs_),
                         0, nullptr, 0, 33, {0, 80});

        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Separator, {0.67f, 0.4f, 0.4f, 1.f});
        ImGui::Separator();
        ImGui::PopStyleColor();

        ImGui::End();
    }

    renderer.projection(space_.projected);
    prototypeRender(renderer);
}

} // namespace hppv
