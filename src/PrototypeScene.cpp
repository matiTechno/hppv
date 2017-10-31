#include <string>

#include <GLFW/glfw3.h>

#include <hppv/PrototypeScene.hpp>
#include <hppv/external/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/App.hpp>

namespace hppv
{

PrototypeScene::PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor):
    prototype_{space, space},
    zoomFactor_(zoomFactor),
    alwaysZoomToCursor_(alwaysZoomToCursor)
{
    properties_.maximize = true;
    rmb_.pressed = false;
}

void PrototypeScene::processInput(bool hasInput)
{
    if(hasInput)
    {
        for(auto& event: frame_.events)
        {
            if(event.type == Event::Cursor)
            {
                if(rmb_.pressed)
                {
                    auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);

                    auto newWorldPos = mapCursor(event.cursor.pos, projection, *this);

                    auto prevWorldPos = mapCursor(rmb_.pos, projection, *this);
                    
                    prototype_.space.pos -= newWorldPos - prevWorldPos;
                }

                rmb_.pos = event.cursor.pos;
            }
            else if(event.type == Event::MouseButton && event.mouseButton.button == GLFW_MOUSE_BUTTON_RIGHT)
            {
                rmb_.pressed = event.mouseButton.action == GLFW_PRESS;
            }
            else if(event.type == Event::Scroll)
            {
                auto zoom = glm::pow(zoomFactor_, event.scroll.offset.y);

                if(rmb_.pressed || alwaysZoomToCursor_)
                {
                    auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);

                    auto worldPos = mapCursor(rmb_.pos, projection, *this);

                    prototype_.space = zoomToPoint(prototype_.space, zoom, worldPos);
                }
                else
                {
                    prototype_.space = zoomToCenter(prototype_.space, zoom);
                }
            }
            else if(event.type == Event::FramebufferSize)
            {
                glm::dvec2 cursorPos;
                glfwGetCursorPos(App::getWindow(), &cursorPos.x, &cursorPos.y);
                rmb_.pos = cursorPos;
            }
        }
    }
    else
    {
        rmb_.pressed = false;
    }

    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);
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

    if(!prototype_.renderImgui)
        return;

    ImGui::Begin("PrototypeScene", nullptr,
                 ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by            m a t i T e c h n o");
        ImGui::Text("frameTime          %f ms", averageFrameTimeMs_);
        ImGui::Text("fps                %d",    averageFps_);
        ImGui::Text("framebuffer size   %d, %d", frame_.framebufferSize.x,
                                                 frame_.framebufferSize.y);

        std::string buttonText("vsync ");
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
        auto spaceCoords = mapCursor(rmb_.pos, projection, *this);
        ImGui::Text("space pos          %.3f, %.3f", spaceCoords.x, spaceCoords.y);
    }
    ImGui::End();
}

} // namespace hppv
