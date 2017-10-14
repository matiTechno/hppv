#pragma once
#include "Scene.hpp"
#include "Space.hpp"

class PrototypeScene: public Scene
{
public:
    PrototypeScene(Space sceneSpace, float zoomFactor);

    void processInput(bool hasInput) final override;
    void render(Renderer& renderer) final override;

private:
    virtual void prototypeProcessInput(bool hasInput) {(void)hasInput;}
    virtual void prototypeRender(Renderer& renderer) {(void)renderer;}

    Space space_;
    float zoomFactor_;
    float zoom_ = 0.f;
    float accumulator_ = 0.f;
    int frameCount_ = 0;
    float averageFrameTimeMs_;
    int averageFps_;
    bool vsync_ = true;
};
