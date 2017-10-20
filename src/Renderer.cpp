#include <cstddef>

#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#define SHADER_IMPLEMENTATION
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Font.hpp>

namespace hppv
{

const char* Renderer::vertexShaderSource = R"(

VERTEX
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 texCoords;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoord;
out vec2 vPosition;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vTexCoord = vertex.zw * texCoords.zw + texCoords.xy;
    vPosition = vertex.xy;
}

)";

static const char* colorShaderSource = R"(

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sampler;
uniform int type;

void main()
{
    color = vColor;
}

)";

static const char* textureShaderSource = R"(

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sampler;
uniform int type;

void main()
{
    color = texture(sampler, vTexCoord) * vColor;
}

)";

static const char* fontShaderSource = R"(

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sampler;
uniform int type;

uniform float smoothing = 1.0 / 16.0;

void main()
{
    color = texture(sampler, vTexCoord) * vColor;

    float distance = texture(sampler, vTexCoord).a;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    color = vec4(vColor.rgb, vColor.a * alpha);
}

)";

static const char* circleShaderSource = R"(

FRAGMENT
#version 330

out vec4 color;

in vec4 vColor;

in vec2 vPosition;

uniform float radius = 0.5;
uniform vec2 center = vec2(0.5, 0.5);

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = smoothstep(radius - delta * 2,
                             radius,
                             distanceFromCenter);

    color = vec4(vColor.rgb, vColor.a * (1 - alpha));
}

)";

Renderer::Renderer():
    shaderColor_(std::string(vertexShaderSource) + colorShaderSource, "Renderer color"),
    shaderTexture_(std::string(vertexShaderSource) + textureShaderSource,
                 "Renderer texture"),
    shaderFont_(std::string(vertexShaderSource) + fontShaderSource, "Renderer font"),
    shaderCircle_(std::string(vertexShaderSource) + circleShaderSource,
            "Renderer circle")
{
    instances_.resize(100000);

    batches_.reserve(100);

    batches_.emplace_back();

    {
        auto& first = batches_.front();
        first.shader = &shaderColor_;
        first.texture = nullptr;
        first.start = 0;
        first.count = 0;
    }

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

void Renderer::setViewport(glm::ivec2 pos, glm::ivec2 size, glm::ivec2 framebufferSize)
{
    auto& batch = getTargetBatch();
    batch.viewportCoords = {pos.x, framebufferSize.y - pos.y - size.y, size.x, size.y};
}

void Renderer::setProjection(const Space& space)
{
    auto& batch = getTargetBatch();
    batch.projection = glm::ortho(space.pos.x, space.pos.x + space.size.x,
                                  space.pos.y + space.size.y, space.pos.y);
}

void Renderer::setShader(sh::Shader* shader)
{
    auto& batch = getTargetBatch();

    if(shader)
        batch.shader = shader;

    else if(batch.texture)
        batch.shader = &shaderTexture_;

    else
        batch.shader = &shaderColor_;
}

void Renderer::setTexture(Texture* texture)
{
    auto& batch = getTargetBatch();
    batch.texture = texture;

    if(batch.shader == &shaderColor_ && texture)
        batch.shader = &shaderTexture_;

    else if(batch.shader == &shaderTexture_ && !texture)
        batch.shader = &shaderColor_;
}

void Renderer::cache(const Sprite& sprite)
{
    Instance i;

    i.color = sprite.color;

    i.matrix = glm::mat4(1.f);

    i.matrix = glm::translate(i.matrix, glm::vec3(sprite.pos, 0.f));

    if(sprite.rotation != 0.f)
    {
        i.matrix = glm::translate(i.matrix, glm::vec3(sprite.size / 2.f
                                                      + sprite.rotationPoint, 0.f));

        i.matrix = glm::rotate(i.matrix, sprite.rotation, glm::vec3(0.f, 0.f, -1.f));

        i.matrix = glm::translate(i.matrix, glm::vec3(-sprite.size / 2.f
                                                      - sprite.rotationPoint, 0.f));
    }

    i.matrix = glm::scale(i.matrix, glm::vec3(sprite.size, 1.f));

    auto& batch = batches_.back();

    if(batch.texture)
    {
        auto texSize = batch.texture->getSize();

        i.texCoords.x = float(sprite.texCoords.x) / texSize.x;
        i.texCoords.y = float(sprite.texCoords.y) / texSize.y;
        i.texCoords.z = float(sprite.texCoords.z) / texSize.x;
        i.texCoords.w = float(sprite.texCoords.w) / texSize.y;
    }

    auto index = batch.start + batch.count;

    if(index + 1 > static_cast<int>(instances_.size()))
        instances_.emplace_back();

    instances_[index] = i;

    ++batch.count;
}

int Renderer::flush()
{
    auto numToRender = batches_.back().start + batches_.back().count;

    if(!numToRender)
        return 0;

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());

    glBufferData(GL_ARRAY_BUFFER, numToRender * sizeof(Instance), instances_.data(),
                 GL_STREAM_DRAW);

    glBindVertexArray(vao_.getId());

    for(auto& batch: batches_)
    {
        glViewport(batch.viewportCoords.x, batch.viewportCoords.y,
                   batch.viewportCoords.z, batch.viewportCoords.w);

        batch.shader->bind();

        glUniformMatrix4fv(batch.shader->getUniformLocation("projection"), 1, GL_FALSE,
                           &batch.projection[0][0]);

        if(batch.texture)
            batch.texture->bind();

        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, batch.count, batch.start);
    }
    
    batches_.erase(batches_.begin(), batches_.end() - 1);
    batches_.front().start = 0;
    batches_.front().count = 0;

    return numToRender;
}

void Renderer::cache(const Text& text)
{
    // todo: move it outside
    setShader(&shaderFont_);
    setTexture(&text.font->getTexture());

    auto& batch = batches_.back();

    auto numInstances = batch.start + batch.count + text.text.size();
    if(numInstances > static_cast<int>(instances_.size()))
        instances_.resize(numInstances);

    auto penPos = text.pos;
    auto index = batch.start + batch.count + 1;

    batch.count += text.text.size() + 1;

    for(auto c: text.text)
    {
        if(c == '\n')
        {
            penPos.x = text.pos.x;
            penPos.y += text.font->getLineHeight() * text.scale;
            continue;
        }

        auto glyph = text.font->getGlyph(c);

        Instance i;
        
        i.color = text.color;

        i.matrix = glm::mat4(1.f);

        i.matrix = glm::translate(i.matrix, glm::vec3(penPos
                    + glm::vec2(glyph.offset) * text.scale, 0.f));
        
        i.matrix = glm::scale(i.matrix, glm::vec3(glm::vec2{glyph.texCoords.z,
                                                   glyph.texCoords.w}
                                                   * text.scale, 1.f));
        
        auto texSize = batch.texture->getSize();

        i.texCoords.x = float(glyph.texCoords.x) / texSize.x;
        i.texCoords.y = float(glyph.texCoords.y) / texSize.y;
        i.texCoords.z = float(glyph.texCoords.z) / texSize.x;
        i.texCoords.w = float(glyph.texCoords.w) / texSize.y;

        instances_[index] = i;
        ++index;
        penPos.x += glyph.advance * text.scale;
    };
}

void Renderer::cache(const Circle& circle)
{

    setShader(&shaderCircle_);

    Instance i;

    i.color = circle.color;

    i.matrix = glm::mat4(1.f);

    i.matrix = glm::translate(i.matrix, glm::vec3(circle.center - circle.radius, 0.f));


    i.matrix = glm::scale(i.matrix, glm::vec3(glm::vec2(circle.radius,
                                                        circle.radius) * 2.f, 1.f));

    auto& batch = batches_.back();

    auto index = batch.start + batch.count;

    if(index + 1 > static_cast<int>(instances_.size()))
        instances_.emplace_back();

    instances_[index] = i;

    ++batch.count;
}

Renderer::Batch& Renderer::getTargetBatch()
{
    {
        auto& current = batches_.back();
        if(!current.count)
            return current;
    }

    batches_.emplace_back();

    auto& current = batches_.back();
    current = *(batches_.end() - 2);
    current.start += current.count;
    current.count = 0;
    return current;
}

} // namespace hppv
