#pragma once

#include <memory> // std::unique_ptr
#include <vector>
#include <string>

#include "Scene.hpp"
#include "Event.hpp"
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
            // see glfwSetInputMode()
            // GLFW_CURSOR_HIDDEN and GLFW_CURSOR_DISABLED disable
            // imgui mouse capture (see ImGui_ImplGlfwGL3_NewFrame())

            int mode;
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
            std::string title = "hppv_test";
            Window::State state = Window::Restored;

            // meaningful only when == Window::Maximized and state == Window::Fullscreen
            Window::State previousState = Window::Restored;
        }
        window;
    };

    bool initialize(const InitParams& initParams);

    void pushScene(std::unique_ptr<Scene> scene) {scenes_.push_back(std::move(scene));}

    void run();

    // executed at the beginning of a frame
    static void request(Request request) {requests_.push_back(request);}

    static const Frame& getFrame() {return frame_;}

    // see Prototype.cpp for proper usage
    static glm::vec2 getCursorPos();

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
    static std::vector<Event> events_;

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
