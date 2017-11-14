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

    void prototypeRenderImGui(bool on) {renderImGui_ = on;}

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
            projected_ = expandToMatchAspectRatio(current, sceneSize);
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

    struct
    {
        bool pressed;
        glm::vec2 pos;
    }
    rmb_;
};

} // namespace hppv
