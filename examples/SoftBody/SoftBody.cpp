#include <algorithm>
#include <fstream>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>

#include "../run.hpp"

struct Data
{
    Data()
    {
        std::ifstream file(filename);
        if(file.is_open())
        {
            file >> k >> mass >> b;
        }
    }

    const char* const filename = "data.txt";
    const glm::vec2 attachmentPoint = {50.f, 50.f};
    const glm::vec2 gravityAcc = {0, 10.f};
    const float timestep = 0.00888f;
    float accumulator = 0.f;
    glm::vec2 point = {60.f, 60.f};
    glm::vec2 vel = {0.f, 0.f};
    float k = 6.8f;
    float mass = 14.4f;
    float b = 2.9f; // coefficient of damping
};

class SoftBody: public hppv::Prototype
{
public:
    SoftBody():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {}

private:
    Data d_;

    void prototypeProcessInput(const bool hasInput) override
    {
        if(!hasInput)
            return;

        if(prototype_.lmb)
        {
            d_.point = hppv::mapCursor(prototype_.cursorPos, space_.projected, this);
            d_.vel = {0.f, 0.f};
        }
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        const auto prevPos = d_.point;

        d_.accumulator += frame_.time;

        // compute

        while(d_.accumulator >= d_.timestep)
        {
            d_.accumulator -= d_.timestep;

            const auto diff = d_.attachmentPoint - d_.point;
            const auto x = glm::length(diff);

            const auto acc = (x * d_.k / d_.mass) * glm::normalize(diff)
                             - (d_.b * d_.vel / d_.mass)
                             + d_.gravityAcc;

            d_.vel += acc * d_.timestep;
            d_.point += d_.vel * d_.timestep;
        }

        // render

        // lerp
        const auto alpha = d_.accumulator / d_.timestep;
        const auto pos = alpha * d_.point + (1.f - alpha) * prevPos;

        {
            renderer.mode(hppv::RenderMode::Vertices);
            renderer.primitive(GL_LINES);
            renderer.shader(hppv::Render::VerticesColor);

            hppv::Vertex vertex;
            vertex.color = {1.f, 0.f, 0.f, 1.f};
            vertex.pos = pos;

            renderer.cache(vertex);

            vertex.pos = d_.attachmentPoint;

            renderer.cache(vertex);
        }
        {
            renderer.mode(hppv::RenderMode::Instances);
            renderer.shader(hppv::Render::CircleColor);

            hppv::Circle circle;
            circle.center = pos;
            circle.radius = 1.f;
            circle.color = {0.f, 1.f, 0.f, 0.f};

            renderer.cache(circle);

            circle.center = d_.attachmentPoint;
            circle.radius = 2.f;
            circle.color = {0.5f, 0.5f, 0.5f, 0.f};

            renderer.cache(circle);
        }

        // imgui

        ImGui::Begin(prototype_.imguiWindowName);
        {
            ImGui::Text("lmb to move object");

            ImGui::InputFloat("k", &d_.k, 0.1f);
            d_.k = std::max(0.01f, d_.k);

            ImGui::InputFloat("mass", &d_.mass, 0.1f);
            d_.mass = std::max(0.01f, d_.mass);

            ImGui::InputFloat("b", &d_.b, 0.1f);
            d_.b = std::max(0.01f, d_.b);

            ImGui::Text("x = %f", glm::length(d_.point - d_.attachmentPoint));
            ImGui::Text("v = %f", glm::length(d_.vel));

            if(ImGui::Button("save"))
            {
                std::ofstream file(d_.filename, std::ios::trunc);
                if(file.is_open())
                {
                    file << d_.k << ' ' << d_.mass << ' ' << d_.b;
                }
            }
        }
        ImGui::End();
    }
};

RUN(SoftBody)
