#pragma once

#include <memory>
#include <vector>
#include "Deleter.hpp"
#include "EventQueue.hpp"
#include <glm/vec2.hpp>

class GLFWwindow;
class Scene;
class Renderer;

struct Frame
{
    float frameTime;
    glm::ivec2 framebufferSize;
    EventQueue eventQueue;
};

class App
{
public:
    App();
    ~App();
    
    bool initialize();

    template<typename T, typename ... Args>
    void executeScene(Args&& ... args)
    {
        scene_ = std::make_unique<T>(std::forward<Args>(args)...);
        run();
    }

    static const Frame& getFrame() {return frame_;}

private:
    Deleter deleterGlfw_;
    GLFWwindow* window_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Renderer> renderer_;
    static Frame frame_;

    void run();
};
