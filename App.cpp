#include "App.hpp"
#include <glm/vec2.hpp>
#include "glad.h"
#include <GLFW/glfw3.h>
#include "Scene.hpp"
#include "Renderer.hpp"
#include "Space.hpp"

App::App() = default;
App::~App() = default;

Frame App::frame_;

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
       frame_.eventQueue.clear();

       glfwPollEvents();

       float newTime = glfwGetTime();
       frame_.frameTime = newTime - time;
       time = newTime;
       
       glfwGetFramebufferSize(window_, &frame_.framebufferSize.x,
                                       &frame_.framebufferSize.y);

       if(scene_->properties_.maximized)
       {
           scene_->properties_.pos = {0, 0};
           scene_->properties_.size = frame_.framebufferSize;
       }
       
       glClear(GL_COLOR_BUFFER_BIT);

       // render scene border
       {
           renderer_->setViewport({0, 0}, frame_.framebufferSize,
                                          frame_.framebufferSize);

           renderer_->setProjection(Space({0, 0}, frame_.framebufferSize));

           auto scenePos = scene_->properties_.pos;
           auto sceneSize = scene_->properties_.size;

           Sprite sprite;
           sprite.color = {0.f, 1.f, 0.f, 0.4f};
           sprite.pos = scenePos - 1;
           sprite.size = sceneSize + 4;
           renderer_->cache(sprite);

           sprite.color = {0.f, 0.f, 0.f, 1.f};
           sprite.pos = scenePos;
           sprite.size = sceneSize;
           renderer_->cache(sprite);

           renderer_->flush();
       }

       renderer_->setViewport(scene_->properties_.pos,
                              scene_->properties_.size, frame_.framebufferSize);

       scene_->render(*renderer_);

       glfwSwapBuffers(window_);
   }
}
