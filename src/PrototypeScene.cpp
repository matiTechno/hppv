#include <string>

#include <GLFW/glfw3.h>

#include <hppv/PrototypeScene.hpp>
#include <hppv/App.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

namespace hppv
{

glm::vec2 getCursorPos()
{
    glm::dvec2 pos;
    glfwGetCursorPos(App::getWindow(), &pos.x, &pos.y);
    return pos;
}

PrototypeScene::PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor):
    prototype_{space, space},
    zoomFactor_(zoomFactor),
    alwaysZoomToCursor_(alwaysZoomToCursor)
{
    properties_.maximize = true;
    rmb_.pressed = false;

    rmb_.pos = getCursorPos();
}

void PrototypeScene::processInput(bool hasInput)
{
    for(auto& event: frame_.events)
    {
        if(event.type == Event::Cursor)
        {
            if(rmb_.pressed && hasInput)
            {
                auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);
                auto newSpaceCoords = mapCursor(event.cursor.pos, projection, this);
                auto prevSpaceCoords = mapCursor(rmb_.pos, projection, this);
                prototype_.space.pos -= newSpaceCoords - prevSpaceCoords;
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
                auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);
                auto spaceCoords = mapCursor(rmb_.pos, projection, this);
                prototype_.space = zoomToPoint(prototype_.space, zoom, spaceCoords);
            }
            else
            {
                prototype_.space = zoomToCenter(prototype_.space, zoom);
            }
        }
        else if(event.type == Event::FramebufferSize)
        {
            rmb_.pos = getCursorPos();
        }
    }

    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    auto projection = expandToMatchAspectRatio(prototype_.space, properties_.size);
    renderer.setProjection(projection);
    prototypeRender(renderer);

    if(!prototype_.renderImgui)
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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.f, 0.f, 0.f, 0.9f});
    ImGui::Begin("PrototypeScene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Text("h p p v");
        ImGui::Text("made by            m a t i T e c h n o");
        ImGui::Text("frameTime          %f ms", averageFrameTimeMs_);
        ImGui::Text("fps                %d",    averageFps_);
        ImGui::Text("framebuffer size   %d, %d", frame_.framebufferSize.x, frame_.framebufferSize.y);

        if(ImGui::Checkbox("vsync", &vsync_))
        {
            App::setVsync(vsync_);
        }

        auto fullscreen = frame_.fullscreen;

        if(ImGui::Checkbox("fullscreen", &fullscreen))
        {
            App::setFullscreen(fullscreen);
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
        auto spaceCoords = mapCursor(rmb_.pos, projection, this);
        ImGui::Text("space coords       %.3f, %.3f", spaceCoords.x, spaceCoords.y);
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

} // namespace hppv
