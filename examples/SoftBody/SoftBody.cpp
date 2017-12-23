#include <algorithm>
#include <fstream>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>

#include "../run.hpp"

class SoftBody: public hppv::Prototype
{
public:
    SoftBody():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {}

private:
    void prototypeRender(hppv::Renderer& renderer)
    {
        // todo: move outside this function
        struct Data
        {
            Data()
            {
                std::ifstream file(filename);
                if(file.is_open())
                {
                    file >> k;
                    file >> mass;
                    file >> b;
                }
            }

            ~Data()
            {
                std::ofstream file(filename, std::ios::trunc);
                if(file.is_open())
                {
                    file << k;
                    file << ' ';
                    file << mass;
                    file << ' ';
                    file << b;
                }
            }

            const char* const filename = "data.txt";
            const glm::vec2 attachmentPoint = {50.f, 50.f};
            float k = 6.8f;
            float mass = 14.4f;
            const glm::vec2 gravityAcc = {0, 10.f};
            glm::vec2 point = {60.f, 60.f};
            glm::vec2 vel = {0.f, 0.f};
            const float timestep = 0.00888f;
            float accumulator = 0.f;
            float b = 2.9f; // coefficient of damping
        }
        static data;

        const auto prevPos = data.point;

        data.accumulator += frame_.time;

        // compute

        while(data.accumulator >= data.timestep)
        {
            data.accumulator -= data.timestep;

            const auto diff = data.attachmentPoint - data.point;
            const auto x = glm::length(diff);

            const auto acc = (x * data.k / data.mass) * glm::normalize(diff)
                             - (data.b * data.vel / data.mass)
                             + data.gravityAcc;

            data.vel += acc * data.timestep;
            data.point += data.vel * data.timestep;
        }

        // render

        // lerp
        const auto alpha = data.accumulator / data.timestep;
        const auto pos = alpha * data.point + (1.f - alpha) * prevPos;

        {
            renderer.mode(hppv::RenderMode::Vertices);
            renderer.primitive(GL_LINES);
            renderer.shader(hppv::Render::VerticesColor);

            hppv::Vertex vertex;
            vertex.color = {1.f, 0.f, 0.f, 1.f};
            vertex.pos = pos;

            renderer.cache(vertex);

            vertex.pos = data.attachmentPoint;

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

            circle.center = data.attachmentPoint;
            circle.radius = 2.f;
            circle.color = {0.5f, 0.5f, 0.5f, 0.f};

            renderer.cache(circle);
        }

        // imgui

        ImGui::Begin(prototype_.imguiWindowName);
        {
            //ImGui::Text("lmb to move object");

            ImGui::InputFloat("k", &data.k, 0.1f);
            data.k = std::max(0.01f, data.k);

            ImGui::InputFloat("mass", &data.mass, 0.1f);
            data.mass = std::max(0.01f, data.mass);

            ImGui::InputFloat("b", &data.b, 0.1f);
            data.b = std::max(0.01f, data.b);

            ImGui::Text("x = %f", glm::length(data.point - data.attachmentPoint));
            ImGui::Text("v = %f", glm::length(data.vel));
        }
        ImGui::End();
    }
};

RUN(SoftBody)
