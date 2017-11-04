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
App::WindowedState App::windowedState_;

bool App::initialize(bool printDebugInfo)
{
    if(printDebugInfo)
    {
        std::cout << "GLFW compile time version " << GLFW_VERSION_MAJOR
                  << '.' << GLFW_VERSION_MINOR << '.' << GLFW_VERSION_REVISION << '\n';

        std::cout << "GLFW run time version     " << glfwGetVersionString() << '\n';

        std::cout << "GLM version               " << GLM_VERSION << '\n';

        std::cout << "dear imgui version        " << IMGUI_VERSION << std::endl;
    }

    glfwSetErrorCallback(errorCallback);

    if(!glfwInit())
        return false;

    deleterGlfw_.set([]{glfwTerminate();});

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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
        std::cout << "OpenGL version            " << glGetString(GL_VERSION)
                  << std::endl;
    }

    glfwSwapInterval(1);

    renderer_ = std::make_unique<Renderer>();

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

    glfwGetFramebufferSize(window_, &frame_.framebufferSize.x,
                                    &frame_.framebufferSize.y);

    scenes_.reserve(ReservedScenes);
    scenesToRender_.reserve(ReservedScenes);
    frame_.events.reserve(ReservedEvents);

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

       for(auto it = scenes_.begin(); it != scenes_.end() - 1; ++it)
       {
           auto& scene = **it;

           if(scene.properties_.maximize)
           {
               scene.properties_.pos = {0, 0};
               scene.properties_.size = frame_.framebufferSize;
           }

           scene.processInput(false);

           if(scene.properties_.updateWhenNotTop)
               scene.update();
       }

       // top scene
       {
           auto imguiWantsInput = ImGui::GetIO().WantCaptureKeyboard ||
                                  ImGui::GetIO().WantCaptureMouse;

           auto& topScene = *scenes_.back();

           if(topScene.properties_.maximize)
           {
               topScene.properties_.pos = {0, 0};
               topScene.properties_.size = frame_.framebufferSize;
           }

           topScene.processInput(!imguiWantsInput);

           topScene.update();
       }

       scenesToRender_.clear();

       for(auto it = scenes_.rbegin(); it != scenes_.rend(); ++it)
       {
           scenesToRender_.insert(scenesToRender_.begin(), &**it);
           if((*it)->properties_.opaque)
               break;
       }

       glClear(GL_COLOR_BUFFER_BIT);

       // borders
       {
           renderer_->setViewport({0, 0, frame_.framebufferSize});

           renderer_->setProjection(Space({0, 0}, frame_.framebufferSize));
           renderer_->setShader(Render::Color);

           for(auto& scene: scenesToRender_)
           {
               auto pos = scene->properties_.pos;
               auto size = scene->properties_.size;

               Sprite sprite;
               sprite.color = {0.f, 1.f, 0.f, 0.4f};
               sprite.pos = pos - 1;
               sprite.size = size + 4;
               renderer_->cache(sprite);

               sprite.color = {0.f, 0.f, 0.f, 1.f};
               sprite.pos = pos;
               sprite.size = size;
               renderer_->cache(sprite);
           }

           renderer_->flush();
       }

       for(auto scene: scenesToRender_)
       {
           renderer_->setViewport(&*scene);

           scene->render(*renderer_);

           renderer_->flush();
       }

       {
           auto& topScene = *scenes_.back();
           auto sceneToPush = std::move(topScene.properties_.sceneToPush);
           auto numToPop = topScene.properties_.numScenesToPop;
           assert(numToPop <= scenes_.size());

           scenes_.erase(scenes_.end() - numToPop, scenes_.end());
           if(sceneToPush) scenes_.push_back(std::move(sceneToPush));
       }

       ImGui::Render();

       glfwSwapBuffers(window_);
   }
}

void App::quit()
{
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void App::setVsync(bool on)
{
    glfwSwapInterval(on);
}

void App::hideCursor(bool hide)
{
    auto mode = hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL;
    glfwSetInputMode(window_, GLFW_CURSOR, mode);
}

void App::setFullscreen(bool on)
{
    if(on)
    {
        glfwGetWindowPos(window_, &windowedState_.pos.x, &windowedState_.pos.y);
        glfwGetWindowSize(window_, &windowedState_.size.x, &windowedState_.size.y);

        auto* monitor = glfwGetPrimaryMonitor();
        const auto* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(window_, nullptr, windowedState_.pos.x, windowedState_.pos.y,
                             windowedState_.size.x, windowedState_.size.y, 0);
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
    frame_.events.push_back(Event(Event::Quit));
}

void App::windowFocusCallback(GLFWwindow*, int focused)
{
    Event event;
    if(focused)
        event.type = Event::FocusGained;
    else
        event.type = Event::FocusLost;

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

void App::framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    Event event(Event::FramebufferSize);
    event.framebufferSize.prevSize = frame_.framebufferSize;
    frame_.framebufferSize = {width, height};
    event.framebufferSize.newSize = frame_.framebufferSize;
    frame_.events.push_back(event);
}

} // namespace hppv
