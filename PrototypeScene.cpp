#include "PrototypeScene.hpp"
#include "imgui/imgui.h"
#include "Renderer.hpp"
#include "App.hpp"
#include <string>
#include <GLFW/glfw3.h>

PrototypeScene::PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor):
    space_(space),
    zoomFactor_(zoomFactor),
    alwaysZoomToCursor_(alwaysZoomToCursor)
{
    properties_.maximize = true;
    rmb_.first = false;
}

void PrototypeScene::processInput(bool hasInput)
{
    if(hasInput)
    {
        for(auto& event: frame_.events)
        {
            if(event.type == Event::Cursor)
            {
                if(rmb_.first)
                {
                    // we need to recreate projection_ before using it
                    // because space_ or scene size might have changed
                    projection_ = expandToMatchAspectRatio(space_, properties_.size);
                    
                    auto newSpaceCoords = cursorSpacePos(projection_, event.cursor.pos,
                                                         *this, frame_.framebufferSize);

                    auto spaceCoords    = cursorSpacePos(projection_, rmb_.second,
                                                         *this, frame_.framebufferSize);
                    
                    space_.pos -= newSpaceCoords - spaceCoords;
                }

                rmb_.second = event.cursor.pos;
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

                if(rmb_.first || alwaysZoomToCursor_)
                {
                    projection_ = expandToMatchAspectRatio(space_, properties_.size);

                    auto spaceCoords = cursorSpacePos(projection_, rmb_.second, *this,
                                                      frame_.framebufferSize);

                    // zoomToCursor would assign modified projection_ to space_
                    // we don't want this
                    space_ = zoomToPoint(space_, zoom, spaceCoords);
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

    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    projection_ = expandToMatchAspectRatio(space_, properties_.size);
    renderer.setProjection(projection_);
    prototypeRender(renderer);
    renderer.flush();

    ++frameCount_;
    accumulator_ += frame_.frameTime;

    if(accumulator_ >= 1.f)
    {
        auto averageFrameTime = accumulator_ / frameCount_;
        averageFrameTimeMs_ = averageFrameTime * 1000.f;
        averageFps_ =   1.f / averageFrameTime + 0.5f;
        frameCount_ = 0;
        accumulator_ = 0.f;
    }

    ImGui::Begin("control panel", nullptr,
                 ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by     m a t i T e c h n o");
        ImGui::Text("frameTime   %f ms", averageFrameTimeMs_);
        ImGui::Text("fps         %d",    averageFps_);

        std::string buttonText("vsyn ");
        if(vsync_)
            buttonText += "off";
        else
            buttonText += "on";

        if(ImGui::Button(buttonText.c_str()))
            App::setVsync(vsync_ = !vsync_);

        ImGui::Separator();
        ImGui::Text("rmb      move around");
        std::string zoomInfo("scroll   zoom to ");
        if(alwaysZoomToCursor_)
            zoomInfo += "cursor";
        else
            zoomInfo += "center / cursor if rmb";
        ImGui::Text(zoomInfo.c_str());

        ImGui::Separator();
        auto spaceCoords = cursorSpacePos(projection_, rmb_.second, *this,
                                          frame_.framebufferSize);
        ImGui::Text("space coords   %.2f, %.2f", spaceCoords.x, spaceCoords.y);
    }
    ImGui::End();
}
