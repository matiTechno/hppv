#include <cstddef>
#include <algorithm>
#include <cassert>

#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#define SHADER_IMPLEMENTATION
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Font.hpp>
#include <hppv/Framebuffer.hpp>

#include "Shaders.hpp"

// see imgui.cpp
int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);

namespace hppv
{

glm::vec2 Text::getSize() const
{
    auto x = 0.f;
    const auto lineHeight = font->getLineHeight() * scale;
    glm::vec2 size(0.f, lineHeight);

    for(const auto c: text)
    {
        if(c == '\n')
        {
            size.x = std::max(size.x, x);
            x = 0.f;
            size.y += lineHeight;
            continue;
        }

        const auto glyph = font->getGlyph(c);
        x += glyph.advance * scale;
    }

    size.x = std::max(size.x, x);
    return size;
}

Renderer::Instance createInstance(const glm::vec2 pos, const glm::vec2 size, const float rotation, const glm::vec2 rotationPoint,
                                  const glm::vec4 color, const glm::vec4 texRect, const glm::vec2 texSize)
{
    Renderer::Instance i;

    i.matrix = glm::mat4(1.f);

    i.matrix = glm::translate(i.matrix, glm::vec3(pos, 0.f));

    if(rotation != 0.f)
    {
        i.matrix = glm::translate(i.matrix, glm::vec3(size / 2.f + rotationPoint, 0.f));
        i.matrix = glm::rotate(i.matrix, rotation, glm::vec3(0.f, 0.f, -1.f));
        i.matrix = glm::translate(i.matrix, glm::vec3(-size / 2.f - rotationPoint, 0.f));
    }

    i.matrix = glm::scale(i.matrix, glm::vec3(size, 1.f));

    i.color = color;

    i.normTexRect.x = texRect.x / texSize.x;
    i.normTexRect.z = texRect.z / texSize.x;
    i.normTexRect.w = texRect.w / texSize.y;
    i.normTexRect.y = 1.f - texRect.y / texSize.y - i.normTexRect.w;

    return i;
}

Renderer::Renderer():
    shaderBasic_({vInstancesSource, fBasicSource}, "hppv::Renderer::shaderBasic_"),
    shaderSdf_({vInstancesSource, fSdfSource}, "hppv::Renderer::shaderSdf_"),
    shaderVertices_({vVerticesSource, fVerticesSource}, "hppv::Renderer::shaderVertices_")
{
    glEnable(GL_BLEND);

    glSamplerParameteri(samplerLinear_.getId(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(samplerLinear_.getId(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glSamplerParameteri(samplerNearest_.getId(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(samplerNearest_.getId(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    {
        const GLuint ids[] = {samplerNearest_.getId(), samplerLinear_.getId()};

        for(const auto id: ids)
        {
            glSamplerParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }

    batches_.reserve(ReservedBatches);
    instances_.resize(ReservedInstances);
    texUnits_.reserve(ReservedTexUnits);
    uniforms_.reserve(ReservedUniforms);
    vertices_.resize(ReservedVertices);

    setTexUnitsDefault();

    batches_.emplace_back();

    {
        auto& batch = batches_.back();
        batch.primitive = GL_TRIANGLES;
        batch.vao = &vaoInstances_;
        batch.shader = &shaderBasic_;
        batch.srcAlpha = GL_ONE;
        batch.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
        batch.premultiplyAlpha = true;
        batch.antialiasedSprites = false;
        batch.flipTexRectX = false;
        batch.flipTexRectY = false;
        batch.flipTextureY = false;
        batch.instances.start = 0;
        batch.instances.count = 0;
        batch.texUnits.start = 1; // first texUnit is omitted, it exists only for texUnits_.back().texture->getSize()
        batch.texUnits.count = 0;
        batch.uniforms.start = 0;
        batch.uniforms.count = 0;
        batch.vertices.start = 0;
        batch.vertices.count = 0;
    }

    float vertices[] =
    {
        0.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 1.f, 1.f, 0.f,
        1.f, 1.f, 1.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    glBindBuffer(GL_ARRAY_BUFFER, boQuad_.getId());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(vaoInstances_.getId());
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

    glBindBuffer(GL_ARRAY_BUFFER, boVertices_.getId());

    glBindVertexArray(vaoVertices_.getId());

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, color)));

    glEnableVertexAttribArray(2);
}

void Renderer::scissor(glm::ivec4 scissor)
{
    scissor.y = App::getFrame().framebufferSize.y - scissor.y - scissor.w;
    getBatchToUpdate().scissor = scissor;
}

void Renderer::viewport(glm::ivec4 viewport)
{
    viewport.y = App::getFrame().framebufferSize.y - viewport.y - viewport.w;
    getBatchToUpdate().viewport = viewport;
}

void Renderer::viewport(const Scene* const scene)
{
    viewport({scene->properties_.pos, scene->properties_.size});
}

void Renderer::viewport(const Framebuffer& framebuffer)
{
    getBatchToUpdate().viewport = {0, 0, framebuffer.getSize()};
}

void Renderer::shader(const Render mode)
{
    const auto modeId = static_cast<int>(mode);
    auto& batch = getBatchToUpdate();

    if(modeId < static_cast<int>(Render::Sdf))
    {
        batch.shader = &shaderBasic_;
    }
    else if(modeId < static_cast<int>(Render::VerticesColor))
    {
        batch.shader = &shaderSdf_;
    }
    else
    {
        batch.shader = &shaderVertices_;
    }

    uniform1i("mode", modeId);
}

void Renderer::uniform1i(const std::string& name, const int value)
{
    uniforms_.emplace_back(Uniform::I1, name);
    uniforms_.back().i1 = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::uniform1f(const std::string& name, const float value)
{
    uniforms_.emplace_back(Uniform::F1, name);
    uniforms_.back().f1 = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::uniform2f(const std::string& name, const glm::vec2 value)
{
    uniforms_.emplace_back(Uniform::F2, name);
    uniforms_.back().f2 = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::uniform3f(const std::string& name, const glm::vec3 value)
{
    uniforms_.emplace_back(Uniform::F3, name);
    uniforms_.back().f3 = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::uniform4f(const std::string& name, const glm::vec4 value)
{
    uniforms_.emplace_back(Uniform::F4, name);
    uniforms_.back().f4 = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::uniformMat4f(const std::string& name, const glm::mat4& value)
{
    uniforms_.emplace_back(Uniform::MAT4F, name);
    uniforms_.back().mat4f = value;
    ++getBatchToUpdate().uniforms.count;
}

void Renderer::texture(Texture& texture, const GLenum unit)
{
    auto& batch = getBatchToUpdate();
    const auto start = batch.texUnits.start;
    const auto count = batch.texUnits.count;

    for(auto i = start; i < start + count; ++i)
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
    last.sampler = (&last - 1)->sampler;
    ++batch.texUnits.count;
}

void Renderer::sampler(GLsampler& sampler, const GLenum unit)
{
    auto& batch = getBatchToUpdate();
    const auto start = batch.texUnits.start;
    const auto count = batch.texUnits.count;

    for(auto i = start; i < start + count; ++i)
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
    last.texture = (&last - 1)->texture;
    ++batch.texUnits.count;
}

void Renderer::cache(const Sprite* sprite, const std::size_t count)
{
    auto& batch = batches_.back();
    assert(batch.vao == &vaoInstances_);
    const auto start = batch.instances.start + batch.instances.count;
    batch.instances.count += count;
    const auto end = batch.instances.start + batch.instances.count;

    if(end > instances_.size())
    {
        instances_.resize(end);
    }

    for(auto i = start; i < end; ++i, ++sprite)
    {
        instances_[i] = createInstance(sprite->pos, sprite->size, sprite->rotation, sprite->rotationPoint,
                                       sprite->color, sprite->texRect, texUnits_.back().texture->getSize());
    }
}

void Renderer::cache(const Circle* circle, const std::size_t count)
{
    auto& batch = batches_.back();
    assert(batch.vao == &vaoInstances_);
    const auto start = batch.instances.start + batch.instances.count;
    batch.instances.count += count;
    const auto end = batch.instances.start + batch.instances.count;

    if(end > instances_.size())
    {
        instances_.resize(end);
    }

    for(auto i = start; i < end; ++i, ++circle)
    {
        instances_[i] = createInstance(circle->center - circle->radius, glm::vec2(circle->radius * 2.f), 0.f, {},
                                       circle->color, circle->texRect, texUnits_.back().texture->getSize());
    }
}

void Renderer::cache(const Text& text)
{
    auto& batch = batches_.back();
    assert(batch.vao == &vaoInstances_);

    {
        const auto end = batch.instances.start + batch.instances.count + text.text.size();

        if(end > instances_.size())
        {
            instances_.resize(end);
        }
    }

    auto penPos = text.pos;
    auto i = batch.instances.start + batch.instances.count;
    const auto halfTextSize = text.getSize() / 2.f;

    for(const auto* s = text.text.data(); *s;)
    {
        unsigned int c;
        s += ImTextCharFromUtf8(&c, s, nullptr);

        if(c == 0)
            break;

        if(c == '\n')
        {
            penPos.x = text.pos.x;
            penPos.y += text.font->getLineHeight() * text.scale;
            continue;
        }

        const auto glyph = text.font->getGlyph(c);

        const auto pos = penPos + glm::vec2(glyph.offset) * text.scale;
        const auto size = glm::vec2(glyph.texRect.z, glyph.texRect.w) * text.scale;

        instances_[i] = createInstance(pos, size, text.rotation, text.rotationPoint + text.pos + halfTextSize - pos
                                       - size / 2.f, // this correction is needed, see createInstance()
                                       text.color, glyph.texRect, texUnits_.back().texture->getSize());

        penPos.x += glyph.advance * text.scale;
        ++i;
        ++batch.instances.count;
    }
}

void Renderer::cache(const Vertex* vertex, const std::size_t count)
{
    auto& batch = batches_.back();
    assert(batch.vao == &vaoVertices_);
    const auto start = batch.vertices.start + batch.vertices.count;
    batch.vertices.count += count;
    const auto end = batch.vertices.start + batch.vertices.count;

    if(end > vertices_.size())
    {
        vertices_.resize(end);
    }

    for(auto i = start; i < end; ++i, ++vertex)
    {
        vertices_[i] = *vertex;
    }
}

void Renderer::flush()
{
    {
        const auto numInstances = batches_.back().instances.start + batches_.back().instances.count;
        glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());
        glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(Instance), instances_.data(), GL_STREAM_DRAW);
    }
    {
        const auto numVertices = batches_.back().vertices.start + batches_.back().vertices.count;
        glBindBuffer(GL_ARRAY_BUFFER, boVertices_.getId());
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(Vertex), vertices_.data(), GL_STREAM_DRAW);
    }

    for(const auto& batch: batches_)
    {
        if(batch.scissor)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(batch.scissor->x, batch.scissor->y, batch.scissor->z, batch.scissor->w);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }

        glViewport(batch.viewport.x, batch.viewport.y, batch.viewport.z, batch.viewport.w);

        auto& shader = *batch.shader;
        shader.bind();

        if(&shader == &shaderBasic_ || &shader == &shaderVertices_)
        {
            shader.uniform1i("premultiplyAlpha", batch.premultiplyAlpha);
        }

        if(&shader == &shaderBasic_)
        {
            shader.uniform1i("antialiasedSprites", batch.antialiasedSprites);
        }

        if(batch.vao == &vaoInstances_)
        {
            shader.uniform1i("flipTexRectX", batch.flipTexRectX);
            shader.uniform1i("flipTexRectY", batch.flipTexRectY);
        }

        shader.uniform1i("flipTextureY", batch.flipTextureY);

        {
            const auto projection = batch.projection;

            const auto matrix = glm::ortho(projection.pos.x, projection.pos.x + projection.size.x,
                                     projection.pos.y + projection.size.y, projection.pos.y);

            shader.uniformMat4f("projection", matrix);
        }

        {
            const auto start = batch.uniforms.start;
            const auto count = batch.uniforms.count;

            for(auto i = start; i < start + count; ++i)
            {
                const auto& uniform = uniforms_[i];
                switch(uniform.type)
                {
                case Uniform::I1: shader.uniform1i(uniform.name, uniform.i1); break;
                case Uniform::F1: shader.uniform1f(uniform.name, uniform.f1); break;
                case Uniform::F2: shader.uniform2f(uniform.name, uniform.f2); break;
                case Uniform::F3: shader.uniform3f(uniform.name, uniform.f3); break;
                case Uniform::F4: shader.uniform4f(uniform.name, uniform.f4); break;
                case Uniform::MAT4F: shader.uniformMat4f(uniform.name, uniform.mat4f);
                }
            }
        }

        {
            const auto start = batch.texUnits.start;
            const auto count = batch.texUnits.count;

            for(auto i = start; i < start + count; ++i)
            {
                auto& unit = texUnits_[i];
                unit.texture->bind(unit.unit);
                glBindSampler(unit.unit, unit.sampler->getId());
            }
        }

        glBlendFunc(batch.srcAlpha, batch.dstAlpha);
        glBindVertexArray(batch.vao->getId());

        if(batch.vao == &vaoInstances_)
        {
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, batch.instances.count, batch.instances.start);
        }
        else
        {
            glDrawArrays(batch.primitive, batch.vertices.start, batch.vertices.count);
        }
    }

    batches_.erase(batches_.begin(), batches_.end() - 1);
    uniforms_.clear();

    {
        auto& batch = batches_.back();
        batch.instances.start = 0;
        batch.instances.count = 0;
        batch.texUnits.start = 1;
        batch.texUnits.count = 0;
        batch.uniforms.start = 0;
        batch.uniforms.count = 0;
        batch.vertices.start = 0;
        batch.vertices.count = 0;
    }

    setTexUnitsDefault();
}

void Renderer::setTexUnitsDefault()
{
    texUnits_.clear();
    texUnits_.emplace_back();
    auto& first = texUnits_.back();
    first.unit = 0;
    first.texture = &texDummy_;
    first.sampler = &samplerLinear_;
}

Renderer::Batch& Renderer::getBatchToUpdate()
{
    {
        auto& current = batches_.back();
        if(!current.instances.count && !current.vertices.count)
            return current;
    }

    batches_.emplace_back();
    auto& current = batches_.back();
    current = *(&current - 1);

    current.instances.start += current.instances.count;
    current.instances.count = 0;
    current.texUnits.start += current.texUnits.count;
    current.texUnits.count = 0;
    current.uniforms.start += current.uniforms.count;
    current.uniforms.count = 0;
    current.vertices.start += current.vertices.count;
    current.vertices.count = 0;

    return current;
}

} // namespace hppv
