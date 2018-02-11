#include <random>
#include <vector>

#include <glm/gtc/matrix_transform.hpp> // glm::ortho
#include <glm/trigonometric.hpp> // glm::sin, glm::cos

#include <hppv/glad.h>
#include <GLFW/glfw3.h>

#include <hppv/App.hpp>
#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/GLobjects.hpp>
#include <hppv/Shader.hpp>
#include <hppv/imgui.h>

const char* const renderSource = R"(

#vertex
#version 330

layout(location = 0) in vec2 currentPos;
layout(location = 1) in vec2 prevPos;

uniform mat4 projection;
uniform float alpha;

out float vPosDelta;

void main()
{
    vec2 pos = alpha * currentPos + (1.0 - alpha) * prevPos;
    gl_Position = projection * vec4(pos, 0.0, 1.0);
    vPosDelta = length(currentPos - prevPos);
}

#fragment
#version 330

in float vPosDelta;

out vec4 color;

void main()
{
    color = vec4(0.3, 0.15, vPosDelta / 8.0, 0.2);
}
)";

const char* const computeSource = R"(

#compute
#version 430

layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer buf0
{
    vec2 currentPos[];
};

layout(std430, binding = 1) buffer buf1
{
    vec2 prevPos[];
};

layout(std430, binding = 2) buffer buf2
{
    vec2 vel[];
};

uniform vec2 gravityPos;
uniform bool isActive;
uniform float dt;

const float mass = 1.0;
const float threshold = 0.00001;
const float gravityCoeff = 100000.0;
const float maxGravity = 100.0;
const float dragCoeff = 0.01;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    vec2 gravity = vec2(0.0);
    vec2 gravityDist = gravityPos - currentPos[id];
    float r = length(gravityDist);

    if(isActive && r > threshold)
    {
        gravity = (gravityDist / r) * min(maxGravity, gravityCoeff / pow(r, 2.0));
    }

    float velLen = length(vel[id]);
    vec2 dragVelDelta = vec2(0.0);

    if(velLen > threshold)
    {
        dragVelDelta = -(vel[id] / velLen) * min(velLen, pow(velLen, 2.0) * dragCoeff / mass * dt);
    }

    prevPos[id] = currentPos[id];
    vel[id] += gravity / mass * dt + dragVelDelta;
    currentPos[id] += vel[id] * dt;
}
)";

void setBuffer(const GLuint boId, const int numParticles, const glm::vec2* const data,
               const GLuint id, const bool vertexAttrib)
{
    glBindBuffer(GL_ARRAY_BUFFER, boId);
    glBufferData(GL_ARRAY_BUFFER, numParticles * sizeof(glm::vec2), data, GL_DYNAMIC_DRAW);

    if(vertexAttrib)
    {
        glVertexAttribPointer(id, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(id);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, boId);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, id, boId);
}

class Gravity: public hppv::Prototype
{
public:
    Gravity():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f}),
        shCompute_({computeSource}, "shCompute_"),
        shRender_({renderSource}, "shRender_")
    {
        static_assert(NumParticles % ComputeLocalSize == 0);

        shCompute_.bind();
        shCompute_.uniform1f("dt", dt_);

        std::vector<glm::vec2> buffer(NumParticles, glm::vec2(0.f));

        glBindVertexArray(vao_.getId());

        setBuffer(boVel_.getId(), NumParticles, buffer.data(), VelId, false);

        // initial positions
        {
            std::random_device rd;
            std::mt19937 rng(rd());
            const auto pos = space_.initial.pos;
            const auto size = space_.initial.size;
            std::uniform_real_distribution<float> distX(pos.x, pos.x + size.x);
            std::uniform_real_distribution<float> distY(pos.y, pos.y + size.y);

            for(auto& vec: buffer)
            {
                vec = {distX(rng), distY(rng)};
            }
        }

        setBuffer(boCurrentPos_.getId(), NumParticles, buffer.data(), CurrentPosId, true);
        setBuffer(boPrevPos_.getId(), NumParticles, nullptr, PrevPosId, true);
    }

private:
    enum
    {
        NumParticles = 1000000,
        ComputeLocalSize = 1000,
        CurrentPosId = 0,
        PrevPosId = 1,
        VelId = 2
    };

    hppv::GLvao vao_;
    hppv::GLbo boVel_, boCurrentPos_, boPrevPos_;
    hppv::Shader shCompute_, shRender_;
    float time_ = 0.f;

    struct
    {
        glm::vec2 pos;
        bool active = false;
    }
    pilot_;

    bool active_ = false;

    // gafferongames.com/post/fix_your_timestep

    float accumulator_ = 0.f;
    const float dt_ = 0.01f;

    void prototypeProcessInput(const hppv::Pinput input) override
    {
        for(const auto& event: input.events)
        {
            if(event.type == hppv::Event::MouseButton && event.mouseButton.action == GLFW_PRESS &&
                                                         event.mouseButton.button == GLFW_MOUSE_BUTTON_LEFT)
            {
                // todo?: hide the input from imgui when active_ == true?
                active_ = !active_;
            }
        }

        time_ += frame_.time;

        {
            const auto space = space_.initial;
            pilot_.pos.x = glm::sin(time_ * 1.2f);
            pilot_.pos.y = glm::cos(time_ * 1.2f);
            pilot_.pos *= space.size / 2.5f;
            pilot_.pos += space.pos + space.size / 2.f;
        }

        shCompute_.bind();

        if(active_ || pilot_.active)
        {
            const glm::vec2 pos = pilot_.active ? pilot_.pos :
                                                  hppv::mapCursor(input.cursorPos, space_.projected, this);

            shCompute_.uniform2f("gravityPos", pos);

            // glsl - error: illegal use of reserved word `active'
            shCompute_.uniform1i("isActive", true);
        }
        else
        {
            shCompute_.uniform1i("isActive", false);
        }

        accumulator_ += frame_.time;
        while(accumulator_ >= dt_)
        {
            accumulator_ -= dt_;
            glDispatchCompute(NumParticles / ComputeLocalSize, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // is this correct?
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    // note: we are rendering outside the Renderer framework
    void prototypeRender(hppv::Renderer&) override
    {
        glEnable(GL_BLEND);

        glViewport(0, 0, properties_.size.x, properties_.size.y);

        shRender_.bind();
        shRender_.uniform1f("alpha", accumulator_ / dt_);

        {
            const auto pos = space_.projected.pos;
            const auto size = space_.projected.size;
            const auto projection = glm::ortho(pos.x, pos.x + size.x, pos.y + size.y, pos.y);
            shRender_.uniformMat4f("projection", projection);
        }

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao_.getId());
        glDrawArrays(GL_POINTS, 0, NumParticles);

        ImGui::Begin(prototype_.imguiWindowName);
        {
            ImGui::Text("lmb - toggle gravity");
            if(ImGui::Checkbox("pilot", &pilot_.active))
            {
                active_ = false;
            }
        }
        ImGui::End();
    }
};

int main()
{
    hppv::App app;
    hppv::App::InitParams p;
    p.glVersion = {4, 3};
    p.window.title = "Gravity";
    p.window.state = hppv::Window::Fullscreen;
    if(!app.initialize(p)) return 1;
    app.pushScene<Gravity>();
    app.run();
    return 0;
}
