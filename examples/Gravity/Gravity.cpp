#include <optional>
#include <random>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>

#include <hppv/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/GLobjects.hpp>
#include <hppv/Shader.hpp>

static const char* renderSource = R"(

#vertex
#version 330

in vec2 pos;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(pos, 0, 1);
}

#fragment
#version 430

out vec4 color;

void main()
{
    color = vec4(0.30, 0.15, 0, 0.1);
}
)";

static const char* computeSource = R"(

#compute
#version 430

layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer buf1
{
    vec2 positions[];
};

layout(std430, binding = 1) buffer buf2
{
    vec2 vels[];
};

uniform vec2 gravityPos;
uniform bool isActive;
uniform float dt;

uniform float dragCoeff = 0.05;
uniform float gravityCoeff = 50000;
uniform float maxGravity = 2000;

void main()
{
    uint id = gl_GlobalInvocationID.x;

    vec2 gravity = vec2(0);

    if(isActive)
    {
        vec2 diff = gravityPos - positions[id];
        gravity = normalize(diff) * min(maxGravity, (1 / pow(length(diff), 2)) * gravityCoeff);
    }

    vec2 drag = clamp(-normalize(vels[id]) * pow(length(vels[id]), 2) * dragCoeff,
                      -abs(vels[id] / dt),
                      abs(vels[id]));

    vec2 force = gravity + drag;
    positions[id] += vels[id] * dt + 0.5 * force * pow(dt, 2);
    vels[id] += force * dt;
}
)";

// todo:
// * fixed dt + position interpolation
// * particle movement is somewhat wrong

class Gravity: public hppv::PrototypeScene
{
public:
    Gravity():
        hppv::PrototypeScene(hppv::Space{0.f, 0.f, 100.f, 100.f}, 1.1f, false),
        shCompute_({computeSource}, "shCompute_"),
        shRender_({renderSource}, "shRender_")
    {
        assert(NumParticles % ComputeLocalGroup == 0);

        std::vector<glm::vec2> buffer(NumParticles);

        // we don't want to divide by 0
        std::fill(buffer.begin(), buffer.end(), glm::vec2(0.00001f));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boVel_.getId());
        glBufferData(GL_SHADER_STORAGE_BUFFER, NumParticles * sizeof(glm::vec2), buffer.data(), GL_DYNAMIC_DRAW);

        {
            std::random_device rd;
            std::mt19937 rng(rd());
            auto pos = space_.initial.pos;
            auto size = space_.initial.size;
            std::uniform_real_distribution<float> distX(pos.x, pos.x + size.x);
            std::uniform_real_distribution<float> distY(pos.y, pos.y + size.y);

            for(auto& vec: buffer)
            {
                vec = {distX(rng), distY(rng)};
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, boPos_.getId());
        glBufferData(GL_ARRAY_BUFFER, NumParticles * sizeof(glm::vec2), buffer.data(), GL_DYNAMIC_DRAW);

        glBindVertexArray(vao_.getId());
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, boPos_.getId());

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, boPos_.getId());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boVel_.getId());
    }

private:
    enum
    {
        NumParticles = 1000000,
        ComputeLocalGroup = 1000
    };

    hppv::GLvao vao_;
    hppv::GLbo boPos_, boVel_;
    hppv::Shader shCompute_, shRender_;

    void prototypeProcessInput(bool hasInput) override
    {
        shCompute_.bind();
        shCompute_.uniform1f("dt", frame_.time);

        if(prototypeLmb() && hasInput)
        {
            shCompute_.uniform2f("gravityPos", hppv::mapCursor(prototypeCursorPos(), space_.projected, this));

            // glsl - error: illegal use of reserved word `active'
            shCompute_.uniform1i("isActive", true);
        }
        else
        {
            shCompute_.uniform1i("isActive", false);
        }

        glDispatchCompute(NumParticles / ComputeLocalGroup, 1, 1);
        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    // note: we are rendering outside the Renderer framework
    void prototypeRender(hppv::Renderer&) override
    {
        glViewport(0, 0, properties_.size.x, properties_.size.y);

        shRender_.bind();

        {
            auto pos = space_.projected.pos;
            auto size = space_.projected.size;
            auto projection = glm::ortho(pos.x, pos.x + size.x, pos.y + size.y, pos.y);
            shRender_.uniformMat4f("projection", projection);
        }

        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao_.getId());
        glDrawArrays(GL_POINTS, 0, NumParticles);
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    // todo: require correct version
    std::cout << "OpenGL version 4.3 is required to run this example" << std::endl;
    app.pushScene<Gravity>();
    app.run();
    return 0;
}
