#include <cstddef>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include <hppv/external/glad.h>
#define SHADER_IMPLEMENTATION
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Space.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Texture.hpp>
#include <hppv/Font.hpp>
#include <hppv/App.hpp>

namespace hppv
{

const std::string Renderer::vertexShaderSource = R"(

#vertex
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 normTexRect;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoords;
out vec2 vPosition;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vTexCoords = vertex.zw * normTexRect.zw + normTexRect.xy;
    vPosition = vertex.xy;
}

)";

const std::string Renderer::vertexShaderTextureFlippedYSource = R"(

#vertex
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 normTexRect;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoords;
out vec2 vPosition;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;

    vec2 texCoords = vertex.zw;
    texCoords.y = (vertex.y - 1) * -1;
    vTexCoords = texCoords.xy * normTexRect.zw + normTexRect.xy;

    vPosition = vertex.xy;
}

)";

static const char* colorShaderSource = R"(

#fragment
#version 330

in vec4 vColor;

out vec4 color;

void main()
{
    color = vColor;
}

)";

static const char* textureShaderSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;

out vec4 color;

void main()
{
    color = texture(sampler, vTexCoords) * vColor;
}

)";

static const char* texturePremultiplyAlphaShaderSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;

out vec4 color;

void main()
{
    vec4 sample = texture(sampler, vTexCoords);
    color = vec4(sample.rgb * sample.a, sample.a) * vColor;
}

)";

static const char* circleColorShaderSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = 1 - smoothstep(radius - delta * 2,
                                 radius,
                                 distanceFromCenter);

    color = vColor * alpha;
}

)";

static const char* circleTextureShaderSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = 1 - smoothstep(radius - delta * 2,
                                 radius,
                                 distanceFromCenter);

    vec4 sample = texture(sampler, vTexCoords);
    color = vec4(sample.rgb * sample.a, sample.a) * vColor * alpha;
}

)";

static const char* fontShaderSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;

const float smoothing = 1.0 / 16.0;

out vec4 color;

void main()
{
    float distance = texture(sampler, vTexCoords).a;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    color = vColor * alpha;
}

)";

Renderer::Renderer()
{
    shaders_.emplace(Render::Color, Shader(vertexShaderSource + colorShaderSource, "Render::Color"));
    shaders_.emplace(Render::Texture, Shader(vertexShaderSource + textureShaderSource, "Render::Texture"));
    shaders_.emplace(Render::TexturePremultiplyAlpha, Shader(vertexShaderSource + texturePremultiplyAlphaShaderSource,
                                                             "Render::Texture"));
    shaders_.emplace(Render::TextureFlippedY, Shader(vertexShaderSource + textureShaderSource, "Render::TextureFlippedY"));
    shaders_.emplace(Render::CircleColor, Shader(vertexShaderSource + circleColorShaderSource, "Render::CircleColor"));
    shaders_.emplace(Render::CircleTexture, Shader(vertexShaderSource + circleTextureShaderSource, "Render::CircleTexture"));
    shaders_.emplace(Render::Font, Shader(vertexShaderSource + fontShaderSource, "Render::Font"));

    instances_.resize(100000);

    glSamplerParameteri(samplerLinear.getId(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(samplerNearest.getId(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    batches_.reserve(100);
    batches_.emplace_back();

    {
        auto& first = batches_.front();
        first.shader = &shaders_.at(Render::Color);
        first.start = 0;
        first.count = 0;
        first.startTexUnit = 0;
        first.countTexUnit = 0;
        first.srcAlpha = GL_ONE;
        first.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
    }

    texUnits_.reserve(100);
    texUnits_.emplace_back();

    {
        auto& first = texUnits_.front();
        first.unit = 0;
        first.texture = &texDummy;
        first.sampler = &samplerLinear;
    }

    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);

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
                          reinterpret_cast<const void*>(offsetof(Instance, normTexRect)));
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

void Renderer::setViewport(glm::ivec4 viewport)
{
    viewport.y = App::getFrame().framebufferSize.y - viewport.y - viewport.w;
    auto& batch = getBatchToUpdate();
    batch.viewport = viewport;
}

void Renderer::setViewport(const Scene& scene)
{
    setViewport({scene.properties_.pos, scene.properties_.size});
}

void Renderer::setScissor(glm::ivec4 scissor)
{
    scissor.y = App::getFrame().framebufferSize.y - scissor.y - scissor.w;
    auto& batch = getBatchToUpdate();
    batch.scissor = scissor;
}

void Renderer::setScissor(const Scene& scene)
{
    setScissor({scene.properties_.pos, scene.properties_.size});
}

void Renderer::setProjection(Space projection)
{
    auto& batch = getBatchToUpdate();
    batch.projection = glm::ortho(projection.pos.x, projection.pos.x + projection.size.x,
                                  projection.pos.y + projection.size.y, projection.pos.y);
}

void Renderer::setShader(Shader& shader)
{
    auto& batch = getBatchToUpdate();
    batch.shader = &shader;
}

void Renderer::setTexture(Texture& texture, GLenum unit)
{
    auto& batch = getBatchToUpdate();

    for(int i = batch.startTexUnit; i < batch.startTexUnit + batch.count; ++i)
    {
        if(texUnits_[i].unit == unit)
        {
            texUnits_[i].texture = &texture;
            return;
        }
    }

    texUnits_.emplace_back();

    auto& last = texUnits_.back();

    last.unit = unit;
    last.texture = &texture;
    last.sampler = (texUnits_.end() - 2)->sampler;

    ++batch.countTexUnit;
}

void Renderer::setSampler(GLsampler& sampler, GLenum unit)
{
    auto& batch = getBatchToUpdate();

    for(int i = batch.startTexUnit; i < batch.startTexUnit + batch.count; ++i)
    {
        if(texUnits_[i].unit == unit)
        {
            texUnits_[i].sampler = &sampler;
            return;
        }
    }

    texUnits_.emplace_back();

    auto& last = texUnits_.back();

    last.unit = unit;
    last.sampler = &sampler;
    last.texture = (texUnits_.end() - 2)->texture;

    ++batch.countTexUnit;
}

void Renderer::setSampler(Sample mode, GLenum unit)
{
    setSampler(mode == Sample::Linear ? samplerLinear : samplerNearest, unit);
}

void Renderer::setShader(Render mode)
{
    auto& batch = getBatchToUpdate();
    batch.shader = &shaders_.at(mode);
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

    auto texSize = texUnits_.back().texture->getSize();

    i.normTexRect.x = sprite.texRect.x / texSize.x;
    i.normTexRect.y = sprite.texRect.y / texSize.y;
    i.normTexRect.z = sprite.texRect.z / texSize.x;
    i.normTexRect.w = sprite.texRect.w / texSize.y;

    auto& batch = batches_.back();

    auto index = batch.start + batch.count;

    if(index + 1 > static_cast<int>(instances_.size()))
        instances_.emplace_back();

    instances_[index] = i;

    ++batch.count;
}

void Renderer::flush()
{
    auto count = batches_.back().start + batches_.back().count;

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());

    glBufferData(GL_ARRAY_BUFFER, count * sizeof(Instance), instances_.data(),
                 GL_STREAM_DRAW);

    glBindVertexArray(vao_.getId());

    for(auto& batch: batches_)
    {
        glScissor(batch.scissor.x, batch.scissor.y,
                   batch.scissor.z, batch.scissor.w);

        glViewport(batch.viewport.x, batch.viewport.y,
                   batch.viewport.z, batch.viewport.w);

        batch.shader->bind();

        glUniformMatrix4fv(batch.shader->getUniformLocation("projection"), 1, GL_FALSE,
                           &batch.projection[0][0]);

        for(int i = batch.startTexUnit; i < batch.startTexUnit + batch.countTexUnit; ++i)
        {
            auto& unit = texUnits_[i];
            unit.texture->bind(unit.unit);
            glBindSampler(unit.unit, unit.sampler->getId());
        }

        glBlendFunc(batch.srcAlpha, batch.dstAlpha);

        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, batch.count, batch.start);
    }

    texUnits_.erase(texUnits_.begin(), texUnits_.end() - batches_.back().countTexUnit);
    batches_.erase(batches_.begin(), batches_.end() - 1);
    batches_.front().start = 0;
    batches_.front().count = 0;
    batches_.front().startTexUnit = 0;
}

void Renderer::cache(const Text& text)
{
    auto& batch = batches_.back();

    auto numInstances = batch.start + batch.count + text.text.size();
    if(numInstances > instances_.size())
        instances_.resize(numInstances);

    auto penPos = text.pos;
    auto index = batch.start + batch.count;

    batch.count += text.text.size();

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
        
        i.matrix = glm::scale(i.matrix, glm::vec3(glm::vec2{glyph.texRect.z,
                                                   glyph.texRect.w}
                                                   * text.scale, 1.f));

        auto texSize = texUnits_.back().texture->getSize();

        i.normTexRect.x = float(glyph.texRect.x) / texSize.x;
        i.normTexRect.y = float(glyph.texRect.y) / texSize.y;
        i.normTexRect.z = float(glyph.texRect.z) / texSize.x;
        i.normTexRect.w = float(glyph.texRect.w) / texSize.y;

        instances_[index] = i;
        ++index;
        penPos.x += glyph.advance * text.scale;
    }
}

void Renderer::cache(const Circle& circle)
{

    Instance i;

    i.color = circle.color;

    i.matrix = glm::mat4(1.f);

    i.matrix = glm::translate(i.matrix, glm::vec3(circle.center - circle.radius, 0.f));


    i.matrix = glm::scale(i.matrix, glm::vec3(glm::vec2(circle.radius,
                                                        circle.radius) * 2.f, 1.f));

    auto texSize = texUnits_.back().texture->getSize();

    i.normTexRect.x = circle.texRect.x / texSize.x;
    i.normTexRect.y = circle.texRect.y / texSize.y;
    i.normTexRect.z = circle.texRect.z / texSize.x;
    i.normTexRect.w = circle.texRect.w / texSize.y;

    auto& batch = batches_.back();

    auto index = batch.start + batch.count;

    if(index + 1 > static_cast<int>(instances_.size()))
        instances_.emplace_back();

    instances_[index] = i;

    ++batch.count;
}

void Renderer::setBlend(GLenum srcAlpha, GLenum dstAlpha)
{
    auto& batch = getBatchToUpdate();
    batch.srcAlpha = srcAlpha;
    batch.dstAlpha = dstAlpha;
}

Renderer::Batch& Renderer::getBatchToUpdate()
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

glm::vec2 Text::getSize() const
{
    float x = 0.f;
    glm::vec2 size = {0.f, 0.f};

    auto lineHeight = font->getLineHeight() * scale;

    size.y += lineHeight;

    for(auto c: text)
    {
        if(c == '\n')
        {
            size.x = std::max(size.x, x);
            x = 0.f;
            size.y += lineHeight;
            continue;
        }

        auto glyph = font->getGlyph(c);
        x += glyph.advance * scale;
    }

    size.x = std::max(size.x, x);
    return size;
}

} // namespace hppv
