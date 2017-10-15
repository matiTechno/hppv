#include "PrototypeScene.hpp"
#include "imgui/imgui.h"
#include "Renderer.hpp"
#include "App.hpp"
#include <string>
#include <GLFW/glfw3.h>

PrototypeScene::PrototypeScene(Space space, float zoomFactor):
    space_(space),
    zoomFactor_(zoomFactor)
{
    properties_.maximize = true;
    rmb_.first = false;
}

void PrototypeScene::processInput(bool hasInput)
{
    space_ = expandToMatchAspectRatio(space_, properties_.size);

    if(hasInput)
    {
        for(auto& event: frame_.events)
        {
            if(event.type == Event::Cursor)
            {
                auto newCursorSpacePos = cursorSpacePos(space_, event.cursor.pos,
                                                        *this, frame_.framebufferSize);

                if(rmb_.first)
                {
                    space_.pos -= newCursorSpacePos - rmb_.second;
                }

                rmb_.second = newCursorSpacePos;
            }
            else if(event.type == Event::MouseButton &&
                    event.mouseButton.button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                if(event.mouseButton.action == GLFW_PRESS)
                {
                    rmb_.first = true;
                }
                else
                {
                    rmb_.first = false;
                }
            }
            else if(event.type == Event::Scroll)
            {
                auto zoom = glm::pow(zoomFactor_, event.scroll.offset.y);

                if(rmb_.first)
                {
                    space_ = zoomToPoint(space_, zoom, rmb_.second);
                }
                else
                {
                    space_ = zoomToCenter(space_, zoom);
                }
            }
        }
    }
    else
    {
        rmb_.first = false;
    }

    glm::dvec2 newCursorPos;
    glfwGetCursorPos(App::getWindow(), &newCursorPos.x, &newCursorPos.y);
    rmb_.second = cursorSpacePos(space_, newCursorPos, *this, frame_.framebufferSize);

    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    renderer.setProjection(space_);
    prototypeRender(renderer);
    renderer.flush();

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

    ImGui::Begin("control panel", nullptr,
                 ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by     m a t i T e c h n o");
        ImGui::Text("frameTime   %f ms", averageFrameTimeMs_);
        ImGui::Text("fps         %d", averageFps_);

        std::string buttonText("vsyn ");
        if(vsync_)
            buttonText += "off";
        else
            buttonText += "on";

        if(ImGui::Button(buttonText.c_str()))
            App::setVsync(vsync_ = !vsync_);

        ImGui::Separator();
        ImGui::Text("rmb      move around\n"
                    "scroll   zoom to center / cursor if rmb");

        ImGui::Separator();
        ImGui::Text("space coords   %.2f, %.2f", rmb_.second.x, rmb_.second.y);
    }
    ImGui::End();
}
