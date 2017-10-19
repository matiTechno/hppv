#include <string>

#include <GLFW/glfw3.h>

#include <hppv/PrototypeScene.hpp>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/App.hpp>

namespace hppv
{

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
                    // we need to recreate projection before using it
                    // because space_ or scene size might have changed
                    auto projection = expandToMatchAspectRatio(space_, properties_.size);
                    
                    auto newSpaceCoords = cursorSpacePos(projection, event.cursor.pos,
                                                         *this);

                    auto prevSpaceCoords = cursorSpacePos(projection, rmb_.second,
                                                          *this);
                    
                    space_.pos -= newSpaceCoords - prevSpaceCoords;
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
                    auto projection = expandToMatchAspectRatio(space_, properties_.size);

                    auto spaceCoords = cursorSpacePos(projection, rmb_.second, *this);

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
    auto projection = expandToMatchAspectRatio(space_, properties_.size);
    renderer.setProjection(projection);
    prototypeRender(renderer);

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

    ImGui::Begin("PrototypeScene", nullptr,
                 ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by            m a t i T e c h n o");
        ImGui::Text("frameTime          %f ms", averageFrameTimeMs_);
        ImGui::Text("fps                %d",    averageFps_);
        ImGui::Text("framebuffer size   %d, %d", frame_.framebufferSize.x,
                                                 frame_.framebufferSize.y);

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
        ImGui::Text("%s", zoomInfo.c_str());

        ImGui::Separator();
        auto spaceCoords = cursorSpacePos(projection, rmb_.second, *this);
        ImGui::Text("space coords       %.3f, %.3f", spaceCoords.x, spaceCoords.y);
    }
    ImGui::End();
}

void PrototypeScene::prototypeProcessInput(bool hasInput)
{
    (void)hasInput;
}

void PrototypeScene::prototypeRender(Renderer& renderer)
{
    (void)renderer;
}

} // namespace hppv
