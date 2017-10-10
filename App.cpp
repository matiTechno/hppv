#include "App.hpp"
#include <glm/vec2.hpp>
#include "glad.h"
#include <GLFW/glfw3.h>
#include "Scene.hpp"
#include "Renderer.hpp"
#include "Rect.hpp"

App::App() = default;
App::~App() = default;

bool App::initialize()
{
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

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return false;

    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    renderer_ = std::make_unique<Renderer>();

    return true;
}

void App::run()
{
   float time = glfwGetTime();

   while(!glfwWindowShouldClose(window_))
   {
       glfwPollEvents();

       float newTime = glfwGetTime();
       float frameTime = newTime - time;
       time = newTime;
       (void)frameTime;
       
       glm::ivec2 framebufferSize;
       glfwGetFramebufferSize(window_, &framebufferSize.x, &framebufferSize.y);
       auto aspect = static_cast<float>(framebufferSize.x) / framebufferSize.y;

       glViewport(0, 0, framebufferSize.x, framebufferSize.y);
       glClear(GL_COLOR_BUFFER_BIT);

       {
           auto camera = scene_->camera_;
           auto cameraAspect = camera.size.x / camera.size.y;

           Rect projection;
           projection.size = camera.size;
       
           if(cameraAspect < aspect)
               projection.size.x = aspect * projection.size.y;
       
           else if(cameraAspect > aspect)
               projection.size.y = projection.size.x / aspect;
       
           projection.pos = camera.pos - (projection.size - camera.size) / 2.f;

           renderer_->setProjection(projection);
           
           scene_->render(*renderer_, projection);
           
           renderer_->flush();
       }

       glfwSwapBuffers(window_);
   }
}
