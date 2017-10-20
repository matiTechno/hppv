#include <vector>
#include <string>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>
#include <hppv/imgui.h>

#include "Shaders.hpp"

class Shaders: public hppv::PrototypeScene
{
public:
    Shaders():
        PrototypeScene(hppv::Space(0.f, 0.f, 0.f, 0.f), 0.f, false)
    {
        addShader(tunnel);
        addShader(lines);
        addShader(explosion);
        addShader(rainbow);
        addShader(hexagons);
        addShader(fractal);
        addShader(cubes);
        addShader(blue);
        addShader(lines2);
        addShader(trinity);
        addShader(storm);
        addShader(curves);
        addShader(waves);
        addShader(fire);
        addShader(octopus);

        activeShader_ = shaders_.begin();
    }

private:
    std::vector<sh::Shader> shaders_;
    std::vector<sh::Shader>::iterator activeShader_;
    float time_ = 0.f;

    void addShader(const char* fragmentSource)
    {
        shaders_.emplace_back(std::string(hppv::Renderer::vertexShaderSource) +
                              "FRAGMENT\n" + fragmentSource, "no id");
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.frameTime;

        ImGui::Begin("next", nullptr,
                     ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("all the shaders come from glslsandbox.com");
        if(ImGui::Button("Next !!!"))
        {
            ++activeShader_;
            if(activeShader_ == shaders_.end())
                activeShader_ = shaders_.begin();
        }
        ImGui::End();

        renderer.setProjection(hppv::Space({0.f, 0.f}, frame_.framebufferSize));
        
        hppv::Sprite sprite;
        sprite.pos = {0.f, 0.f};
        sprite.size = frame_.framebufferSize;

        activeShader_->bind();

        glUniform1f(activeShader_->getUniformLocation("time"), time_);

        glUniform2f(activeShader_->getUniformLocation("resolution"),
                    float(frame_.framebufferSize.x), float(frame_.framebufferSize.y));

        renderer.setShader(&*activeShader_);
        renderer.cache(sprite);
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<Shaders>();
    app.run();
    return 0;
}
