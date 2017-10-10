#pragma once

#include <memory>
#include "Deleter.hpp"

class GLFWwindow;
class Scene;
class Renderer;

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

private:
    Deleter deleterGlfw_;
    GLFWwindow* window_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Renderer> renderer_;

    void run();
};
