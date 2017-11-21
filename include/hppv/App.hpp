#pragma once

#include <memory>
#include <vector>

#include "Frame.hpp"
#include "Deleter.hpp"

struct GLFWwindow;

namespace hppv
{

class Scene;
class Renderer;

// note: glfw does not detect if window is fullscreen when set
// externally by the window manager (xfce4, i3) and there is no way to
// restore it from the application

// if windowState == Restored and window was maximized before
// being fullscreen it will be set to maximized

struct Request
{
    enum Type
    {
        Quit,
        Vsync,
        Cursor,
        Window
    }
    type;

    Request(Type type): type(type) {}
    Request() = default;

    union
    {
        struct
        {
            bool visible;
        }
        cursor;

        struct
        {
            bool on;
        }
        vsync;

        struct
        {
            Frame::Window::State state;
        }
        window;
    };
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

    // executed at the end of a frame
    static void request(Request request) {requests_.push_back(request);}

    // internal use
    static const Frame& getFrame() {return frame_;}

private:
    enum
    {
        ReservedScenes = 10,
        ReservedEvents = 100,
        ReservedRequests = 5
    };

    Deleter deleterGlfw_;
    Deleter deleterImGui_;
    std::unique_ptr<Renderer> renderer_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    std::vector<Scene*> scenesToRender_;
    static GLFWwindow* window_;
    static Frame frame_;
    static bool handleQuitEvent_;
    static std::vector<Request> requests_;

    static void refreshFrame();

    static void errorCallback(int, const char* description);
    static void windowCloseCallback(GLFWwindow*);
    static void windowFocusCallback(GLFWwindow*, int focused);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow*, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);
    static void framebufferSizeCallback(GLFWwindow*, int, int);
};

} // namespace hppv
