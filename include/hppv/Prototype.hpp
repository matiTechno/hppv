#pragma once

#include <utility>

#include "Scene.hpp"
#include "Space.hpp"

namespace hppv
{

class PSpace
{
public:
    PSpace(Space initial):
        initial(initial),
        current_(initial)
    {}

    void set(Space space)
    {
        current_ = space;
        projected_ = expandToMatchAspectRatio(space, sceneSize_);
    }

    // Prototype calls it
    void newFrame(glm::ivec2 sceneSize)
    {
        sceneSize_ = sceneSize;
        projected_ = expandToMatchAspectRatio(current_, sceneSize);
    }

    const Space initial;
    const Space& current = current_;
    const Space& projected = projected_;

private:
    Space current_;
    Space projected_;
    glm::ivec2 sceneSize_;
};

struct PInput
{
    const std::vector<Event> events;
    glm::vec2 cursorPos;
    bool lmb;
};

class Prototype: public Scene
{
public:
    Prototype(Space space);

    void processInput(const std::vector<Event>& events) final override;
    void render(Renderer& renderer)  final override;

protected:
    PSpace space_;

    struct
    {
        const char* const imguiWindowName = "Prototype";
        bool renderImgui = true;
        float zoomFactor = 1.1f;
        bool alwaysZoomToCursor = false;
    }
    prototype_;

private:
    // todo: callbacks?
    virtual void prototypeProcessInput(PInput) {}

    // projection is already set
    virtual void prototypeRender(Renderer&) {}

    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float avgFrameTimeMs_ = 0.f;
    int avgFps_ = 0;
    float avgFrameTimesMs_[180];

    // we should get the value from App
    bool vsync_ = true;

    bool rmb_ = false;
    bool lmb_ = false;

    // event based unlike App::getCursorPos()
    glm::vec2 cursorPos_;
};

} // namespace hppv
