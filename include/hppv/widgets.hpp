#pragma once

// Scene.hpp already includes this file

// todo?: better filename?

namespace hppv
{

struct Frame;

template<typename T, int N>
constexpr int size(T(&)[N])
{
    return N;
}

// frameTime
// fps
// framebuffer size
// vsync, quit
// fullscreen, maximize, restore
// frameTime plot
// spacing

class AppWidget
{
public:
    AppWidget();

    void update(const Frame& frame);

    // call inside the ImGui::Begin() ImGui::End() block
    void imgui(const Frame& frame) const;

private:
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float avgFrameTimeMs_ = 0.f;
    float avgFrameTimesMs_[180];
};

void imguiPushDisabled(float alpha = 0.5f);
void imguiPopDisabled();

} // namespace hppv
