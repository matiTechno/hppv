#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

// Scene.hpp already includes this file

// todo?: better filename?

namespace hppv
{

struct Frame;
struct Event;

template<typename T, int N>
constexpr int size(T(&)[N])
{
    return N;
}

void imguiPushDisabled(float alpha = 0.5f);
void imguiPopDisabled();

class AppWidget
{
public:
    void update(const Frame& frame);

    // call inside the ImGui::Begin() ImGui::End() block
    void imgui(const Frame& frame) const;

private:
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float frameTimesMs_[180] = {};
};

// keys used: Esc, w, s, a, d, space, lshift
// bug: jumps on window resize
class Camera
{
public:
    Camera();

    // todo: better naming (function and vars)
    // == capture mouse
    void enterCameraMode();

    void processEvent(const Event& event);

    // process events first
    // assumes properties_.size == frame.framebufferSize
    void update(const Frame& frame);

    void imgui() const;

    // todo?: direction?
    // note: watch out for var shadowing
    glm::vec3 eye = {0.f, 0.5f, 3.f};
    glm::vec3 up = {0.f, 1.f, 0.f};
    float fovy = 90.f; // degrees
    float zNear = 0.1f;
    float zFar = 10000.f;
    float speed = 4.f;
    float sensitivity = 0.1f;

    // degrees
    float pitch = 0.f;
    float yaw = -90.f; // -z direction

    // get after update()
    glm::mat4 view;
    glm::mat4 projection;

private:
    enum
    {
        Forward,
        Back,
        Left,
        Right,
        Up,
        Down,
        ToggleCameraMode,
        NumControls
    };

    int controls_[NumControls];

    struct
    {
        // initialized to false
        bool pressed[NumControls] = {};
        bool held[NumControls] = {};
    }
    keys_;

    glm::vec2 cursorPos_;
    bool cameraMode_;

    void toggleCameraMode();
    bool active(int control) const;
};

} // namespace hppv
