#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Frame.hpp"
#include "Deleter.hpp"

struct GLFWwindow;

namespace hppv
{

class Scene;

// note: glfw does not detect if the window is fullscreen when set
// externally by the window manager (i3, xfce4)

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

    Request() = default;
    Request(Type type): type(type) {}

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
            // if the requested window state == Window::Restored and
            // the current window state == Window::Fullscreen and
            // the previous window state == Window::Maximized,
            // the possible new window state will be Window::Maximized

            Window::State state;
        }
        window;
    };
};

class App
{
public:
    struct InitParams
    {
        bool printDebugInfo = false;
        bool handleQuitEvent = true;

        struct
        {
            int major = 3;
            int minor = 3;
        }
        glVersion;

        struct
        {
            glm::ivec2 size = {640, 480};
            std::string title = "hppv";
            Window::State state = Window::Restored;

            // meaningful only when == Window::Maximized and state == Window::Fullscreen
            Window::State previousState = Window::Restored;
        }
        window;
    };

    bool initialize(const InitParams& initParams);

    template<typename T, typename ... Args>
    void pushScene(Args&& ... args)
    {
        scenes_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void run();

    // executed at the beginning of a frame
    static void request(Request request) {requests_.push_back(request);}

    static const Frame& getFrame() {return frame_;}

private:
    enum
    {
        ReservedScenes = 10,
        ReservedEvents = 100,
        ReservedRequests = 5
    };

    Deleter deleterGlfw_;
    Deleter deleterImgui_;
    std::vector<std::unique_ptr<Scene>> scenes_;
    static GLFWwindow* window_;
    static Frame frame_;
    static bool handleQuitEvent_;
    static std::vector<Request> requests_;

    static void refreshFrame();

    static void setFullscreen();

    static void handleRequests();

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
