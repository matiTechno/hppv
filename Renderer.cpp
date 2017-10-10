#include "glad.h"
#define SHADER_IMPLEMENTATION
#include "Shader.hpp"
#include "Renderer.hpp"
#include <cassert>
#include <cstddef>
#include <glm/gtc/matrix_transform.hpp>

static const char* shaderSource = R"(

VERTEX
#version 330

layout(location = 0) in vec2 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;

void main()
{
    gl_Position = projection * matrix * vec4(vertex, 0, 1);
    vColor = color;
}

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;

void main()
{
    color = vColor;
}

)";

Renderer::Renderer():
    shader_(shaderSource, "Renderer")
{
    instances_.reserve(100000);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &boQuad_);
    glGenBuffers(1, &boInstances_);

    float vertices[] =
    {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,
        1.f, 1.f,
        0.f, 1.f,
        0.f, 0.f
    };

    glBindBuffer(GL_ARRAY_BUFFER, boQuad_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vao_);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, color)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)));

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + sizeof(glm::vec4)));

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + 2 * sizeof(glm::vec4)));

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + 3 * sizeof(glm::vec4)));
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &boQuad_);
    glDeleteBuffers(1, &boInstances_);
}

void Renderer::setProjection(Rect rect)
{
    assert(instances_.empty());

    auto matrix = glm::ortho(rect.pos.x, rect.pos.x + rect.size.x,
                             rect.pos.y + rect.size.y, rect.pos.y);

    shader_.bind();
    glUniformMatrix4fv(shader_.getUniformLocation("projection"), 1, GL_FALSE,
                       &matrix[0][0]);
}

int Renderer::flush()
{
    glBindBuffer(GL_ARRAY_BUFFER, boInstances_);

    glBufferData(GL_ARRAY_BUFFER, instances_.size() * sizeof(Instance),
                 instances_.data(), GL_STREAM_DRAW);

    glBindVertexArray(vao_);

    shader_.bind();

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instances_.size());

    auto numRendered = instances_.size();
    instances_.clear();
    return numRendered;
}


void Renderer::cache(const Sprite& sprite)
{
    cache(sprite.pos, sprite.size, sprite.color, sprite.rotation,
            sprite.rotationPoint);
}

void Renderer::cache(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotation,
        glm::vec2 rotationPoint)
{
    Instance instance;

    instance.color = color;

    auto& matrix = instance.matrix;

    matrix = glm::mat4(1.f);

    matrix = glm::translate(matrix, glm::vec3(pos, 0.f));

    if(rotation)
    {
        matrix = glm::translate(matrix, glm::vec3(size / 2.f + rotationPoint, 0.f));
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.f, 0.f, -1.f));
        matrix = glm::translate(matrix, glm::vec3(-size / 2.f - rotationPoint, 0.f));
    }

    matrix = glm::scale(matrix, glm::vec3(size, 1.f));

    instances_.push_back(instance);
}
