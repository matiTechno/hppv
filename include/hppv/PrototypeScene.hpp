#pragma once

#include <utility>

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
    struct
    {
        const Space initialSpace;
        Space space;
        bool renderImgui = true;
    }
    prototype_;

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
    bool vsync_ = true;

    struct
    {
        bool pressed;
        glm::vec2 pos;
    }
    rmb_;
};

} // namespace hppv
