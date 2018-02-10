#include <cstring> // std::memset, std::memmove

#include <hppv/widgets.hpp>
#include <hppv/Frame.hpp>
#include <hppv/App.hpp>
#include <hppv/imgui.h>

// ImGui::PushItemFlag()
#include "imgui/imgui_internal.h"

namespace hppv
{

AppWidget::AppWidget()
{
    std::memset(avgFrameTimesMs_, 0, sizeof(avgFrameTimesMs_));
}

void AppWidget::update(const Frame& frame)
{
    ++frameCount_;
    accumulator_ += frame.time;

    if(accumulator_ >= 0.0333f)
    {
        const auto avgFrameTime = accumulator_ / frameCount_;
        avgFrameTimeMs_ = avgFrameTime * 1000.f;
        frameCount_ = 0;
        accumulator_ = 0.f;
        std::memmove(avgFrameTimesMs_, avgFrameTimesMs_ + 1, sizeof(avgFrameTimesMs_) - sizeof(float));
        avgFrameTimesMs_[size(avgFrameTimesMs_) - 1] = avgFrameTimeMs_;
    }
}

void request(const Window::State state)
{
    Request r(Request::Window);
    r.window.state = state;
    App::request(r);
}

void requestVsync(bool on)
{
    Request r(Request::Vsync);
    r.vsync.on = on;
    App::request(r);
}

void AppWidget::imgui(const Frame& frame) const
{
    ImGui::Text("frameTime          %f ms", avgFrameTimeMs_);
    ImGui::Text("fps                %d",    static_cast<int>(1.f / avgFrameTimeMs_ * 1000.f + 0.5f));
    ImGui::Text("framebuffer size   %d, %d", frame.framebufferSize.x, frame.framebufferSize.y);
    ImGui::Spacing();
    ImGui::Text("vsync");

    ImGui::SameLine();
    if(ImGui::Button("on ")) requestVsync(true);

    ImGui::SameLine();
    if(ImGui::Button("off")) requestVsync(false);

    ImGui::SameLine(240);
    if(ImGui::Button("quit")) App::request(Request::Quit);

    ImGui::Spacing();
    {
        const auto isFullscreen = frame.window.state == Window::Fullscreen;
        if(isFullscreen) imguiPushDisabled();
        if(ImGui::Button("fullscreen")) request(Window::Fullscreen);
        if(isFullscreen) imguiPopDisabled();
    }
    ImGui::SameLine();
    {
        const auto isNotRestored = frame.window.state != Window::Restored;
        if(isNotRestored) imguiPushDisabled();
        if(ImGui::Button("maximize")) request(Window::Maximized);
        if(isNotRestored) imguiPopDisabled();
    }
    ImGui::SameLine();
    {
        const auto isRestored = frame.window.state == Window::Restored;
        if(isRestored) imguiPushDisabled();
        if(ImGui::Button("restore")) request(Window::Restored);
        if(isRestored) imguiPopDisabled();
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_PlotLines, {1.f, 1.f, 0.f, 1.f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, {1.f, 0.8f, 0.8f, 0.07f});
    ImGui::PlotLines("frameTime(ms)", avgFrameTimesMs_, size(avgFrameTimesMs_),
                     0, nullptr, 0, 33, {0, 80});

    ImGui::PopStyleColor(2);
    ImGui::Spacing();
}

void imguiPushDisabled(const float alpha)
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * alpha);
}

void imguiPopDisabled()
{
    ImGui::PopStyleVar();
    ImGui::PopItemFlag();
}

} // namespace hppv
