#pragma once

#include <utility>

#include "Scene.hpp"
#include "Space.hpp"

namespace hppv
{

class PrototypeScene: public Scene
{
public:
    PrototypeScene(glm::ivec4 world, float zoomFactor, bool alwaysZoomToCursor);

    void processInput(bool hasInput) final override;
    void render(Renderer& renderer)  final override;

protected:
    struct
    {
        const Space initialWorld;
        Space world;
        Space projection;
    }
    prototype_;

private:
    virtual void prototypeProcessInput(bool hasInput) {(void)hasInput;}

    // projection is already set
    virtual void prototypeRender(Renderer& renderer) {(void)renderer;}

    float zoomFactor_;
    bool alwaysZoomToCursor_;
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float averageFrameTimeMs_ = 0.f;
    int averageFps_ = 0;
    bool vsync_ = true;
    std::pair<bool, glm::vec2> rmb_;
};

} // namespace hppv
