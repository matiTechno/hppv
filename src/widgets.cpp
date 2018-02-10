#include <algorithm> // std::copy, std::max

#include <hppv/widgets.hpp>
#include <hppv/Frame.hpp>
#include <hppv/App.hpp>
#include <hppv/imgui.h>

// ImGui::PushItemFlag()
#include "imgui/imgui_internal.h"

namespace hppv
{

void AppWidget::update(const Frame& frame)
{
    ++frameCount_;
    accumulator_ += frame.time;

    if(accumulator_ >= 0.033f)
    {
        auto* const end = frameTimesMs_ + size(frameTimesMs_);
        std::copy(frameTimesMs_ + 1, end, frameTimesMs_);
        *(end - 1) = accumulator_ / frameCount_ * 1000.f;
        frameCount_ = 0;
        accumulator_ = 0.f;
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
    {
        auto max = 0.f;
        auto sum = 0.f;

        for(const auto v: frameTimesMs_)
        {
            sum += v;
            max = std::max(max, v);
        }

        const auto avg = sum / size(frameTimesMs_);

        ImGui::Text("frame time ms");
        ImGui::PushStyleColor(ImGuiCol_Text, {0.f, 1.f, 0.f, 1.f});
        ImGui::Text("avg   %.3f (%d)", avg, static_cast<int>(1.f / avg * 1000.f + 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, {1.f, 0.f, 0.f, 1.f});
        ImGui::Text("max   %.3f", max);
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_PlotLines, {1.f, 1.f, 0.f, 1.f});
    ImGui::PushStyleColor(ImGuiCol_FrameBg, {1.f, 0.8f, 0.8f, 0.07f});
    ImGui::PlotLines("", frameTimesMs_, size(frameTimesMs_), 0, nullptr, 0, 33, {0, 80});
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
