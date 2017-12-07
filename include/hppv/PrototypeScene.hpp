#pragma once

#include <utility>

#include <glm/vec2.hpp>

#include "Scene.hpp"
#include "Space.hpp"

namespace hppv
{

class PrototypeScene: public Scene
{
public:
    PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor);

    void processInput(bool hasInput) final override;
    void render(Renderer& renderer)  final override;

protected:
    // this needs a better design
    void prototypeRenderImGui(bool on) {renderImGui_ = on;}
    bool prototypeLmb() const {return lmb_;}
    glm::vec2 prototypeCursorPos() const {return cursorPos_;}

    class PrototypeSpace
    {
    public:
        PrototypeSpace(Space initial):
            initial(initial),
            current(current_),
            projected(projected_),
            current_(initial)
        {}

        void set(Space space)
        {
            current_ = space;
            projected_ = expandToMatchAspectRatio(space, sceneSize_);
        }

        void newFrame(glm::ivec2 sceneSize)
        {
            sceneSize_ = sceneSize;
            projected_ = expandToMatchAspectRatio(current_, sceneSize);
        }

        const Space initial;
        const Space& current;
        const Space& projected;

    private:
        Space current_;
        Space projected_;
        glm::ivec2 sceneSize_;
    }
    space_;

private:
    virtual void prototypeProcessInput(bool hasInput) {(void)hasInput;}

    // projection is already set
    virtual void prototypeRender(Renderer& renderer) {(void)renderer;}

    const float zoomFactor_;
    const bool alwaysZoomToCursor_;
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float averageFrameTimeMs_ = 0.f;
    int averageFps_ = 0;
    bool renderImGui_ = true;

    // we should get the value from App
    bool vsync_ = true;

    bool rmb_ = false;
    bool lmb_ = false;

    // event based unlike frame_.cursorPos
    glm::vec2 cursorPos_;
};

} // namespace hppv
