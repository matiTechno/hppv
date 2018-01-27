#include <iostream>
#include <cassert>

#include <hppv/glad.h> // must be included before glfw3.h
#include <GLFW/glfw3.h>

#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Space.hpp>
#include <hppv/imgui.h>

#include "imgui/imgui_impl_glfw_gl3.h"

namespace hppv
{

GLFWwindow* App::window_;
Frame App::frame_;
bool App::handleQuitEvent_;
std::vector<Request> App::requests_;

bool App::initialize(const InitParams& initParams)
{
    if(initParams.printDebugInfo)
    {
        std::cout << "GLFW compile time version " << GLFW_VERSION_MAJOR
                  << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << '\n'
                  << "GLFW run time version     " << glfwGetVersionString() << '\n'
                  << "GLM version               " << GLM_VERSION << '\n'
                  << "dear imgui version        " << IMGUI_VERSION << std::endl;
    }

    glfwSetErrorCallback(errorCallback);

    if(!glfwInit())
        return false;

    deleterGlfw_.set([]{glfwTerminate();});

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, initParams.glVersion.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, initParams.glVersion.minor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if(initParams.window.state == Window::Maximized)
    {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    }

    window_ = glfwCreateWindow(initParams.window.size.x, initParams.window.size.y, initParams.window.title.c_str(),
                               nullptr, nullptr);

    if(!window_)
        return false;

    glfwMakeContextCurrent(window_);

    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << "gladLoadGLLoader() failed" << std::endl;
        return false;
    }

    if(initParams.printDebugInfo)
    {
        std::cout << "vendor                    " << glGetString(GL_VENDOR) << '\n';
        std::cout << "renderer                  " << glGetString(GL_RENDERER) << '\n';
        std::cout << "OpenGL version            " << glGetString(GL_VERSION) << std::endl;
    }

    if(GLAD_GL_ARB_texture_storage == false)
    {
        std::cout << "ARB_texture_storage extension is not supported" << std::endl;
        return false;
    }

    if(GLAD_GL_ARB_base_instance == false)
    {
        std::cout << "ARB_base_instance extension is not supported" << std::endl;
        return false;
    }

    glfwSwapInterval(1);

    ImGui_ImplGlfwGL3_Init(window_, false);
    deleterImgui_.set([]{ImGui_ImplGlfwGL3_Shutdown();});

    glfwSetWindowCloseCallback(window_, windowCloseCallback);
    glfwSetWindowFocusCallback(window_, windowFocusCallback);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetCharCallback(window_, charCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    scenes_.reserve(ReservedScenes);
    frame_.events.reserve(ReservedEvents);
    requests_.reserve(ReservedRequests);

    handleQuitEvent_ = initParams.handleQuitEvent;

    frame_.window.restored.pos = {0, 0};
    frame_.window.restored.size = initParams.window.size;

    // todo: setting the fullscreen mode in glfwCreateWindow()?
    // (note: glfwRestoreWindow() will not set the correct window size in one case)

    if(initParams.window.state == Window::Fullscreen)
    {
        setFullscreen();
    }

    refreshFrame();
    frame_.window.previousState = initParams.window.previousState;

    return true;
}

void App::run()
{
    Renderer renderer;
    std::vector<Scene*> scenesToRender;
    scenesToRender.reserve(ReservedScenes);

    auto time = glfwGetTime();

    while(scenes_.size())
    {
        handleRequests();

        if(glfwWindowShouldClose(window_))
            break;

        frame_.events.clear();

        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        {
            const auto newTime = glfwGetTime();
            frame_.time = newTime - time;
            time = newTime;
        }

        refreshFrame();

        const auto imguiWantsInput = ImGui::GetIO().WantCaptureKeyboard ||
                                     ImGui::GetIO().WantCaptureMouse;

        for(auto it = scenes_.begin(); it != scenes_.end(); ++it)
        {
            auto& scene = **it;

            if(scene.properties_.maximize)
            {
                scene.properties_.pos = {0, 0};
                scene.properties_.size = frame_.framebufferSize;
            }

            const auto isTop = it == scenes_.end() - 1;

            scene.processInput(isTop && !imguiWantsInput);

            if(scene.properties_.updateWhenNotTop || isTop)
            {
                scene.update();
            }
        }

        scenesToRender.clear();

        for(auto it = scenes_.crbegin(); it != scenes_.crend(); ++it)
        {
            scenesToRender.insert(scenesToRender.begin(), &**it);

            if((*it)->properties_.opaque)
                break;
        }

        glClear(GL_COLOR_BUFFER_BIT);

        for(auto scene: scenesToRender)
        {
            renderer.viewport(&*scene);
            scene->render(renderer);
            renderer.flush();
        }

        ImGui::Render();

        glfwSwapBuffers(window_);

        auto& topScene = *scenes_.back();
        auto sceneToPush = std::move(topScene.properties_.sceneToPush);

        {
            const auto numToPop = topScene.properties_.numScenesToPop;
            assert(numToPop <= scenes_.size());
            scenes_.erase(scenes_.end() - numToPop, scenes_.end());
        }

        if(sceneToPush)
        {
            scenes_.push_back(std::move(sceneToPush));
        }
    }
}

void App::refreshFrame()
{
    if(glfwGetWindowAttrib(window_, GLFW_FOCUSED))
    {
        glm::dvec2 cursorPos;
        glfwGetCursorPos(window_, &cursorPos.x, &cursorPos.y);
        frame_.cursorPos = cursorPos;
    }

    glfwGetFramebufferSize(window_, &frame_.framebufferSize.x, &frame_.framebufferSize.y);

    const auto previousState = frame_.window.state;

    if(glfwGetWindowMonitor(window_))
    {
        frame_.window.state = Window::Fullscreen;
    }
    else if(glfwGetWindowAttrib(window_, GLFW_MAXIMIZED))
    {
        frame_.window.state = Window::Maximized;
    }
    else
    {
        frame_.window.state = Window::Restored;
        glfwGetWindowPos(window_, &frame_.window.restored.pos.x, &frame_.window.restored.pos.y);
        glfwGetWindowSize(window_, &frame_.window.restored.size.x, &frame_.window.restored.size.y);
    }

    if(frame_.window.state != previousState)
    {
        frame_.window.previousState = previousState;
    }
}

void App::setFullscreen()
{
    auto* const monitor = glfwGetPrimaryMonitor();
    const auto* const mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
}

void App::handleRequests()
{
    for(const auto& request: requests_)
    {
        switch(request.type)
        {
        case Request::Quit: glfwSetWindowShouldClose(window_, GLFW_TRUE); break;

        case Request::Vsync: glfwSwapInterval(request.vsync.on); break;

        case Request::Cursor:
        {
            const auto mode = request.cursor.visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;
            glfwSetInputMode(window_, GLFW_CURSOR, mode);
            break;
        }

        case Request::Window:
        {
            const auto state = request.window.state;
            assert(frame_.window.state != state);

            if(state == Window::Fullscreen)
            {
                setFullscreen();
            }
            else if(state == Window::Maximized)
            {
                assert(frame_.window.state != Window::Fullscreen);
                glfwMaximizeWindow(window_);
            }
            else
            {
                if(frame_.window.state == Window::Fullscreen)
                {
                    glfwSetWindowMonitor(window_, nullptr, frame_.window.restored.pos.x, frame_.window.restored.pos.y,
                                         frame_.window.restored.size.x, frame_.window.restored.size.y, 0);

                    if(frame_.window.previousState == Window::Maximized)
                    {
                        glfwMaximizeWindow(window_);
                    }
                }
                else
                {
                    glfwRestoreWindow(window_);
                }
            }
        }
        }
    }

    requests_.clear();
}

void App::errorCallback(int, const char* const description)
{
    std::cout << description << std::endl;
}

void App::windowCloseCallback(GLFWwindow*)
{
    if(handleQuitEvent_)
        return;

    glfwSetWindowShouldClose(window_, GLFW_FALSE);
    frame_.events.push_back(Event::Quit);
}

void App::windowFocusCallback(GLFWwindow*, const int focused)
{
    Event event;

    if(focused)
    {
        event.type = Event::FocusGained;
    }
    else
    {
        event.type = Event::FocusLost;
    }

    frame_.events.push_back(event);
}

void App::keyCallback(GLFWwindow* const window, const int key, const int scancode, const int action, const int mods)
{
    Event event(Event::Key);
    event.key.key = key;
    event.key.action = action;
    event.key.mods = mods;
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}

void App::cursorPosCallback(GLFWwindow*, const double xpos, const double ypos)
{
    Event event(Event::Cursor);
    event.cursor.pos = {xpos, ypos};
    frame_.events.push_back(event);
}

void App::mouseButtonCallback(GLFWwindow* const window, const int button, const int action, const int mods)
{
    Event event(Event::MouseButton);
    event.mouseButton.button = button;
    event.mouseButton.action = action;
    event.mouseButton.mods = mods;
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
}

void App::scrollCallback(GLFWwindow* const window, const double xoffset, const double yoffset)
{
    Event event(Event::Scroll);
    event.scroll.offset = {xoffset, yoffset};
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}

void App::charCallback(GLFWwindow* const window, const unsigned int codepoint)
{
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
}

void App::framebufferSizeCallback(GLFWwindow*, int, int)
{
    frame_.events.push_back(Event::FramebufferSize);
}

} // namespace hppv
