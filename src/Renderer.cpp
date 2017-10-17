#include <cassert>
#include <cstddef>

#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#define SHADER_IMPLEMENTATION
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>
#include <hppv/App.hpp>
#include <hppv/Texture.hpp>

static const char* shaderSource = R"(

VERTEX
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 texCoords;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoord;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vTexCoord = vertex.zw * texCoords.zw + texCoords.xy;
}

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sampler;
uniform int type;

void main()
{
    if(type == 0)
        color = vColor;

    else if(type == 1)
        color = texture(sampler, vTexCoord) * vColor;
}

)";

namespace hppv
{

Renderer::Renderer():
    shader_(shaderSource, "Renderer")
{
    instances_.resize(100000);
    batches_.reserve(100);

    float vertices[] =
    {
        0.f, 0.f, 0.f, 0.f,
        1.f, 0.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 0.f, 0.f, 0.f
    };

    glBindBuffer(GL_ARRAY_BUFFER, boQuad_.getId());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vao_.getId());
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, color)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, texCoords)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)));

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + sizeof(glm::vec4)));

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + 2 * sizeof(glm::vec4)));

    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          reinterpret_cast<const void*>(offsetof(Instance, matrix)
                          + 3 * sizeof(glm::vec4)));
}

void Renderer::setProjection(const Space& space)
{
    //assert(instances_.empty());

    auto matrix = glm::ortho(space.pos.x, space.pos.x + space.size.x,
                             space.pos.y + space.size.y, space.pos.y);

    shader_.bind();
    glUniformMatrix4fv(shader_.getUniformLocation("projection"), 1, GL_FALSE,
                       &matrix[0][0]);
}

void Renderer::setViewport(glm::ivec2 framebufferSize)
{
    //assert(instances_.empty());

    glViewport(0, 0, framebufferSize.x, framebufferSize.y);
}

void Renderer::setViewport(const Scene& scene)
{
    //assert(instances_.empty());

    auto pos = scene.properties_.pos;
    auto size = scene.properties_.size;

    glViewport(pos.x, scene.frame_.framebufferSize.y - pos.y - size.y, size.x, size.y);
}

int Renderer::flush()
{
    if(batches_.empty())
        return 0;

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());

    auto numToRender = batches_.back().start + batches_.back().count; 

    glBufferData(GL_ARRAY_BUFFER, numToRender * sizeof(Instance), instances_.data(),
                 GL_STREAM_DRAW);

    glBindVertexArray(vao_.getId());

    shader_.bind();

    for(auto& batch: batches_)
    {
        if(batch.texture)
            batch.texture->bind();

        glUniform1i(shader_.getUniformLocation("type"), 1);
        
        glDrawArraysInstanced(GL_TRIANGLES, batch.start, 6, batch.count);
    }
    
    batches_.clear();

    return numToRender;
}


void Renderer::cache(const Sprite& sprite)
{
    cache(sprite.pos, sprite.size, sprite.color, sprite.rotation,
          sprite.rotationPoint, sprite.texture, sprite.texCoords);
}

void Renderer::cache(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotation,
                     glm::vec2 rotationPoint, Texture* texture, glm::ivec4 texCoords)
{
    auto start = 0;
    auto type = 0;

    if(batches_.size())
    {
        auto& lastBatch = batches_.back();
        start = lastBatch.start + lastBatch.count;
        if(start > instances_.size())
            instances_.resize(start + 1);
        if(texture != lastBatch.texture)
        {
            if(texture)
            type = 1;
            else
                type = 0;
            goto addBatch;
        }
    }
    else
    {
addBatch:
        batches_.push_back(Batch{type, texture, start, 0});
    }

    ++batches_.back().count;

    auto& instance = instances_[start];

    instance.color = color;

    auto& matrix = instance.matrix;

    matrix = glm::mat4(1.f);

    matrix = glm::translate(matrix, glm::vec3(pos, 0.f));

    if(rotation != 0.f)
    {
        matrix = glm::translate(matrix, glm::vec3(size / 2.f + rotationPoint, 0.f));
        matrix = glm::rotate(matrix, rotation, glm::vec3(0.f, 0.f, -1.f));
        matrix = glm::translate(matrix, glm::vec3(-size / 2.f - rotationPoint, 0.f));
    }

    matrix = glm::scale(matrix, glm::vec3(size, 1.f));

    if(texture)
    {
        auto texSize = texture->getSize();

        instance.texCoords.x = float(texCoords.x) / texSize.x;
        instance.texCoords.y = float(texCoords.y) / texSize.y;
        instance.texCoords.z = float(texCoords.z) / texSize.x;
        instance.texCoords.w = float(texCoords.w) / texSize.y;
    }
}

} // namespace hppv
