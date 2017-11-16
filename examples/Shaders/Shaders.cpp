#include <vector>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>

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
    std::vector<hppv::Shader> shaders_;
    std::vector<hppv::Shader>::iterator activeShader_;
    float time_ = 0.f;

    void addShader(const char* fragmentSource)
    {
        shaders_.emplace_back(hppv::Shader({hppv::Renderer::vInstancesSource, fragmentSource}, ""));
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.frameTime;

        ImGui::Begin("next", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        {
            ImGui::Text("all the shaders come from glslsandbox.com");

            if(ImGui::Button("Next !!!"))
            {
                ++activeShader_;

                if(activeShader_ == shaders_.end())
                {
                    activeShader_ = shaders_.begin();
                }
            }
        }
        ImGui::End();

        renderer.shader(*activeShader_);
        renderer.uniform1f("time", time_);
        renderer.uniform2f("resolution", properties_.size);
        renderer.projection(hppv::Sprite());
        renderer.cache(hppv::Sprite());
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
