#pragma once

#include <utility>

#include <glm/vec2.hpp>

#include "Scene.hpp"
#include "Space.hpp"

namespace hppv
{

class PrototypeSpace
{
public:
    PrototypeSpace(Space initial):
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

class Prototype: public Scene
{
public:
    Prototype(Space space);

    void processInput(bool hasInput) final override;
    void render(Renderer& renderer)  final override;

protected:
    PrototypeSpace space_;

    struct
    {
        const bool& lmb; // might be true even if the scene has no input
        const glm::vec2& cursorPos;
        const char* const imguiWindowName = "Prototype";
        bool renderImgui = true;
        float zoomFactor = 1.1f;
        bool alwaysZoomToCursor = false;
    }
    prototype_;

private:
    virtual void prototypeProcessInput(bool hasInput) {(void)hasInput;}

    // projection is already set
    virtual void prototypeRender(Renderer& renderer) {(void)renderer;}

    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float avgFrameTimeMs_ = 0.f;
    int avgFps_ = 0;
    float avgFrameTimesMs_[180];

    // we should get the value from App
    bool vsync_ = true;

    bool rmb_ = false;
    bool lmb_ = false;

    // event based unlike frame_.cursorPos
    glm::vec2 cursorPos_;
};

} // namespace hppv
