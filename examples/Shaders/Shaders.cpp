#include <vector>

#include <hppv/Prototype.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>
#include <hppv/Renderer.hpp>
#include <hppv/Shader.hpp>

#include "Shaders.hpp"
#include "../run.hpp"

class Shaders: public hppv::Prototype
{
public:
    Shaders():
        Prototype({0.f, 0.f, 1.f, 1.f})
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

    void addShader(const char* const fragmentSource)
    {
        shaders_.emplace_back(hppv::Shader({hppv::Renderer::vInstancesSource, fragmentSource}, ""));
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        time_ += frame_.time;

        ImGui::Begin(prototype_.imguiWindowName);
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
        renderer.cache(hppv::Sprite(space_.projected));
    }
};

RUN(Shaders)
