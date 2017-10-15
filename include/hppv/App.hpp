#pragma once

#include <memory>
#include <vector>
#include "Deleter.hpp"
#include "Event.hpp"
#include <glm/vec2.hpp>

class GLFWwindow;
class Scene;
class Renderer;

struct Frame
{
    float frameTime;
    glm::ivec2 framebufferSize;
    std::vector<Event> events;
};

class App
{
public:
    App();
    ~App();
    
    // sets frame_.framebufferSize
    bool initialize();

    template<typename T, typename ... Args>
    void executeScene(Args&& ... args)
    {
        scene_ = std::make_unique<T>(std::forward<Args>(args)...);
        run();
    }

    static const Frame& getFrame() {return frame_;}
    static GLFWwindow* getWindow() {return window_;}
    static void quit();
    static void setVsync(bool on);

private:
    Deleter deleterGlfw_;
    Deleter deleterImGui_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Renderer> renderer_;
    static GLFWwindow* window_;
    static Frame frame_;
    static bool handleQuitEvent_;

    void run();
    static void errorCallback(int, const char* description);
    static void windowCloseCallback(GLFWwindow*);
    static void windowFocusCallback(GLFWwindow*, int focused);
    static void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow*, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow*, int button, int action, int mods);
    static void scrollCallback(GLFWwindow*, double xoffset, double yoffset);
    static void charCallback(GLFWwindow*, unsigned int codepoint);
    static void framebufferSizeCallback(GLFWwindow*, int width, int height);
};
