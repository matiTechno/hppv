#include <algorithm> // std::copy, std::max

#include <GLFW/glfw3.h>

#include <glm/trigonometric.hpp> // glm::sin, glm::cos, glm::radians
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/widgets.hpp>
#include <hppv/Frame.hpp>
#include <hppv/App.hpp>
#include <hppv/imgui.h>
#include <hppv/Event.hpp>

// ImGui::PushItemFlag()
#include "imgui/imgui_internal.h"

namespace hppv
{

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
        ImGui::PushStyleColor(ImGuiCol_Text, {0.f, 0.85f, 0.f, 1.f});
        ImGui::Text("avg   %.3f (%d)", avg, static_cast<int>(1.f / avg * 1000.f + 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, {0.9f, 0.f, 0.f, 1.f});
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

Camera::Camera()
{
    controls_[Forward] = GLFW_KEY_W;
    controls_[Back] = GLFW_KEY_S;
    controls_[Left] = GLFW_KEY_A;
    controls_[Right] = GLFW_KEY_D;
    controls_[Up] = GLFW_KEY_SPACE;
    controls_[Down] = GLFW_KEY_LEFT_SHIFT;
    controls_[ToggleCameraMode] = GLFW_KEY_ESCAPE;
}

void Camera::enterCameraMode()
{
    cameraMode_ = false;
    toggleCameraMode();
}

void Camera::processEvent(const Event& event)
{
    if(event.type == Event::Key)
    {
        auto idx = -1;
        for(auto i = 0; i < NumControls; ++i)
        {
            if(event.key.key == controls_[i])
            {
                idx = i;
                break;
            }
        }

        if(idx == -1)
            return;

        if(event.key.action == GLFW_PRESS)
        {
            keys_.pressed[idx] = true;
            keys_.held[idx] = true;
        }
        else if(event.key.action == GLFW_RELEASE)
        {
            keys_.held[idx] = false;
        }
    }
    else if(event.type == Event::Cursor && cameraMode_)
    {
        const auto offset = event.cursor.pos - cursorPos_;
        cursorPos_ = event.cursor.pos;
        pitch -= offset.y * sensitivity;
        // todo: why 89 and not 90? (learnopengl.com/Getting-started/Camera)
        pitch = glm::min(89.f, pitch);
        pitch = glm::max(-89.f, pitch);
        yaw = glm::mod(yaw + offset.x * sensitivity, 360.f);
    }
}

void Camera::update(const Frame& frame)
{
    if(keys_.pressed[ToggleCameraMode]) toggleCameraMode();

    const auto dir = glm::normalize(glm::vec3(glm::cos(glm::radians(pitch)) * glm::cos(glm::radians(yaw)),
                                              glm::sin(glm::radians(pitch)),
                                              glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw))));
    {
        glm::vec3 moveDir(0.f);

        if(active(Forward)) moveDir += dir;
        if(active(Back)) moveDir -= dir;
        {
            const auto right = glm::normalize(glm::cross(dir, up));
            if(active(Left)) moveDir -= right;
            if(active(Right)) moveDir += right;
        }
        if(active(Up)) moveDir += up;
        if(active(Down)) moveDir -= up;

        if(glm::length(moveDir) != 0.f)
        {
            glm::normalize(moveDir);
        }

        eye += moveDir * speed * frame.time;
    }

    for(auto& key: keys_.pressed)
    {
        key = false;
    }

    const auto aspectRatio = static_cast<float>(frame.framebufferSize.x) / frame.framebufferSize.y;
    projection = glm::perspective(glm::radians(fovy), aspectRatio, zNear, zFar);
    view = glm::lookAt(eye, eye + dir, up);
}

void Camera::imgui() const
{
    ImGui::Text("toggle the camera mode - Esc");
    ImGui::Text("pitch / yaw - mouse");
    ImGui::Text("move - wsad, space (up), lshift (down)");
    // todo: position, ...
}

void Camera::toggleCameraMode()
{
    cameraMode_ = !cameraMode_;
    Request r(Request::Cursor);
    r.cursor.mode = cameraMode_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
    App::request(r);
    cursorPos_ = App::getCursorPos();
}

bool Camera::active(const int control) const
{
    return keys_.pressed[control] || keys_.held[control];
}

} // namespace hppv
