#include <algorithm>
#include <vector>
#include <string>

#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Shader.hpp>
#include <hppv/imgui.h>

#include "Glsl.hpp"

class GLTransitions: public hppv::Scene
{
public:
    GLTransitions():
        tex1_("res/mononoke.jpg"),
        tex2_("res/laputa.jpg")
    {
        properties_.maximize = true;

        addShader(burn, "burn");
        addShader(heart, "heart");
        addShader(cube, "cube");
        addShader(pixelize, "pixelize");
        addShader(ripple, "ripple");
        addShader(hexagonalize, "hexagonalize");

        for(auto& sh: shaders_)
        {
            sh.bind();
            sh.uniform1i("samplerFrom", 0);
            sh.uniform1i("samplerTo", 1);
        }
    }

private:
    hppv::Texture tex1_, tex2_;
    hppv::Texture* texFrom_ = &tex1_;
    hppv::Texture* texTo_ = &tex2_;
    std::vector<hppv::Shader> shaders_;
    std::vector<const char*> shNames_;

    struct
    {
        int id = 0;
        float maxTime = 2.f;
        float progressTime = 0.f;
    }
    transition_;

    void addShader(const std::string& transition, const char* name)
    {
        shaders_.push_back(hppv::Shader({hppv::Renderer::vInstancesSource,
                                         fragStart + transition + fragEnd},
                                        name));

        shNames_.push_back(name);
    }

    void startTransition()
    {
        transition_.progressTime = 0.f;
        std::swap(texFrom_, texTo_);
    }

    void render(hppv::Renderer& renderer) override
    {
        transition_.progressTime += frame_.frameTime;

        ImGui::Begin("transition");
        {
            ImGui::Text("github.com/gl-transitions/gl-transitions");
            ImGui::Separator();

            if(ImGui::Combo("transition", &transition_.id, shNames_.data(), shNames_.size()))
            {
                startTransition();
            }

            ImGui::SliderFloat("maxTime", &transition_.maxTime, 0.f, 5.f);

            if(ImGui::Button("go!"))
            {
                startTransition();
            }
        }
        ImGui::End();

        hppv::Sprite sprite;
        sprite.pos = {0.f, 0.f};
        sprite.size = texTo_->getSize();
        sprite.texRect = sprite.toVec4();

        renderer.projection(hppv::expandToMatchAspectRatio(sprite.toSpace(), properties_.size));
        renderer.texture(*texFrom_, 0);
        renderer.texture(*texTo_, 1);
        renderer.uniform1f("progress", std::min(1.f, transition_.progressTime / transition_.maxTime));
        renderer.uniform1f("ratio", static_cast<float>(properties_.size.x) / properties_.size.y);
        renderer.shader(shaders_[transition_.id]);
        renderer.cache(sprite);
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<GLTransitions>();
    app.run();
    return 0;
}
