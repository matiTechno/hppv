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

App::App() = default;
App::~App() = default;

GLFWwindow* App::window_;
Frame App::frame_;
bool App::handleQuitEvent_ = true;
std::vector<Request> App::requests_;

bool App::initialize(bool printDebugInfo)
{
    if(printDebugInfo)
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(640, 480, "test", nullptr, nullptr);

    if(!window_)
        return false;

    glfwMakeContextCurrent(window_);

    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        return false;

    if(printDebugInfo)
    {
        std::cout << "vendor                    " << glGetString(GL_VENDOR) << '\n';
        std::cout << "renderer                  " << glGetString(GL_RENDERER) << '\n';
        std::cout << "OpenGL version            " << glGetString(GL_VERSION) << std::endl;
    }

    glfwSwapInterval(1);

    renderer_ = std::make_unique<Renderer>();

    ImGui_ImplGlfwGL3_Init(window_, false);
    deleterImGui_.set([]{ImGui_ImplGlfwGL3_Shutdown();});

    glfwSetWindowCloseCallback(window_, windowCloseCallback);
    glfwSetWindowFocusCallback(window_, windowFocusCallback);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwSetCharCallback(window_, charCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    scenes_.reserve(ReservedScenes);
    scenesToRender_.reserve(ReservedScenes);
    frame_.events.reserve(ReservedEvents);
    requests_.reserve(ReservedRequests);

    refreshFrame();
    frame_.window.previousState = frame_.window.state;

    return true;
}

void App::run()
{
   float time = glfwGetTime();

   while(!glfwWindowShouldClose(window_) && scenes_.size())
   {
       frame_.events.clear();

       glfwPollEvents();
       ImGui_ImplGlfwGL3_NewFrame();

       float newTime = glfwGetTime();
       frame_.frameTime = newTime - time;
       time = newTime;

       refreshFrame();

       auto imguiWantsInput = ImGui::GetIO().WantCaptureKeyboard ||
                              ImGui::GetIO().WantCaptureMouse;

       for(auto it = scenes_.begin(); it != scenes_.end(); ++it)
       {
           auto& scene = **it;

           if(scene.properties_.maximize)
           {
               scene.properties_.pos = {0, 0};
               scene.properties_.size = frame_.framebufferSize;
           }

           auto isTop = it == scenes_.end() - 1;

           scene.processInput(isTop && !imguiWantsInput);

           if(scene.properties_.updateWhenNotTop || isTop)
           {
               scene.update();
           }
       }

       scenesToRender_.clear();

       for(auto it = scenes_.rbegin(); it != scenes_.rend(); ++it)
       {
           scenesToRender_.insert(scenesToRender_.begin(), &**it);

           if((*it)->properties_.opaque)
               break;
       }

       glClear(GL_COLOR_BUFFER_BIT);

       for(auto scene: scenesToRender_)
       {
           renderer_->viewport(&*scene);
           scene->render(*renderer_);
           renderer_->flush();
       }

       ImGui::Render();

       glfwSwapBuffers(window_);

       {
           auto& topScene = *scenes_.back();
           auto sceneToPush = std::move(topScene.properties_.sceneToPush);
           auto numToPop = topScene.properties_.numScenesToPop;
           assert(numToPop <= scenes_.size());
           scenes_.erase(scenes_.end() - numToPop, scenes_.end());

           if(sceneToPush)
           {
               scenes_.push_back(std::move(sceneToPush));
           }
       }

       for(auto& request: requests_)
       {
           switch(request.type)
           {
           case Request::Quit: glfwSetWindowShouldClose(window_, GLFW_TRUE); break;

           case Request::Vsync: glfwSwapInterval(request.vsync.on); break;

           case Request::Cursor:
           {
               auto mode = request.cursor.visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN;
               glfwSetInputMode(window_, GLFW_CURSOR, mode);
               break;
           }

           case Request::Window:
           {
               auto state = request.window.state;
               assert(frame_.window.state != state);

               if(state == Window::Fullscreen)
               {
                   auto* monitor = glfwGetPrimaryMonitor();
                   const auto* mode = glfwGetVideoMode(monitor);
                   glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
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

    auto previousState = frame_.window.state;

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

void App::errorCallback(int, const char* description)
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

void App::windowFocusCallback(GLFWwindow*, int focused)
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

void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Event event(Event::Key);
    event.key.key = key;
    event.key.action = action;
    event.key.mods = mods;
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}

void App::cursorPosCallback(GLFWwindow*, double xpos, double ypos)
{
    Event event(Event::Cursor);
    event.cursor.pos = {xpos, ypos};
    frame_.events.push_back(event);
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Event event(Event::MouseButton);
    event.mouseButton.button = button;
    event.mouseButton.action = action;
    event.mouseButton.mods = mods;
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
}

void App::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Event event(Event::Scroll);
    event.scroll.offset = {xoffset, yoffset};
    frame_.events.push_back(event);

    ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}

void App::charCallback(GLFWwindow* window, unsigned int codepoint)
{
    ImGui_ImplGlfwGL3_CharCallback(window, codepoint);
}

void App::framebufferSizeCallback(GLFWwindow*, int, int)
{
    frame_.events.push_back(Event::FramebufferSize);
}

} // namespace hppv
