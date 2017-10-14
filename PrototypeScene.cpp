#include "PrototypeScene.hpp"
#include "imgui/imgui.h"
#include "Renderer.hpp"
#include "App.hpp"
#include <string>

PrototypeScene::PrototypeScene(Space sceneSpace, float zoomFactor):
    space_(sceneSpace),
    zoomFactor_(zoomFactor)
{
    properties_.maximize = true;
}

void PrototypeScene::processInput(bool hasInput)
{
    (void)hasInput;
    // ...
    prototypeProcessInput(hasInput);
}

void PrototypeScene::render(Renderer& renderer)
{
    space_ = expandToMatchAspectRatio(space_, properties_.size);
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
                    "scrool   zoom to center / cursor if rmb");
    }
    ImGui::End();
}
