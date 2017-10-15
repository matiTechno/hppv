#pragma once
#include "Scene.hpp"
#include "Space.hpp"
#include <utility>

class PrototypeScene: public Scene
{
public:
    PrototypeScene(Space space, float zoomFactor, bool alwaysZoomToCursor);

    void processInput(bool hasInput) final override;
    void render(Renderer& renderer)  final override;

private:
    virtual void prototypeProcessInput(bool hasInput) {(void)hasInput;}
    virtual void prototypeRender(Renderer& renderer)  {(void)renderer;}

    Space space_, projection_;
    float zoomFactor_;
    bool alwaysZoomToCursor_;
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float averageFrameTimeMs_ = 0.f;
    int averageFps_ = 0;
    bool vsync_ = true;
    std::pair<bool, glm::vec2> rmb_;
};
