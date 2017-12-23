#include <hppv/Scene.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

class Imgui: public hppv::Scene
{
public:
    void render(hppv::Renderer&) override
    {
        ImGui::ShowTestWindow();
    }
};

RUN(Imgui)
