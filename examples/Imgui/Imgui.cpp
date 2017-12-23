#include <hppv/Scene.hpp>
#include <hppv/imgui.h>
#include <hppv/Texture.hpp>

#include "../run.hpp"

class Imgui: public hppv::Scene
{
public:
    Imgui():
        tex1_("dindinwo.jpg"),
        tex2_("hamburger.png")
    {}

    void render(hppv::Renderer&) override
    {
        ImGui::Begin("image test");
        {
            const glm::vec2 size = tex1_.getSize();
            ImGui::Image(reinterpret_cast<void*>(tex1_.getId()), {size.x, size.y}, {0, 1}, {1, 0});
        }
        {
            const glm::vec2 size = tex2_.getSize();
            ImGui::Image(reinterpret_cast<void*>(tex2_.getId()), {size.x, size.y}, {0, 1}, {1, 0},
            {1.f, 1.f, 1.f, 1.f}, {0.f, 1.f, 0.f, 1.f});
        }
        ImGui::End();

        ImGui::ShowTestWindow();
    }

private:
    hppv::Texture tex1_, tex2_;
};

RUN(Imgui)
