#pragma once

#include <memory>
#include <vector>

#include <glm/vec2.hpp>

#include "Deleter.hpp"
#include "Event.hpp"

class GLFWwindow;

namespace hppv
{

class Scene;
class Renderer;

struct Frame
{
    float frameTime;
    glm::ivec2 framebufferSize; // initialized in App::initialize()
    std::vector<Event> events;
};

class App
{
public:
    App();
    ~App();
    
    bool initialize(bool printDebugInfo);

    template<typename T, typename ... Args>
    void pushScene(Args&& ... args)
    {
        scenes_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void run();

    static void quit();
    static void setVsync(bool on);
    static const Frame& getFrame() {return frame_;}
    static GLFWwindow* getWindow() {return window_;}

private:
    Deleter deleterGlfw_;
    Deleter deleterImgui_;
    std::unique_ptr<Renderer> renderer_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    std::vector<Scene*> scenesToRender_;
    static GLFWwindow* window_;
    static Frame frame_;
    static bool handleQuitEvent_;

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

} // namespace hppv
