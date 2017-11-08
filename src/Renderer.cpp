#include <cstddef>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include <hppv/glad.h>
#define SHADER_IMPLEMENTATION
#include <hppv/Shader.hpp> // must be included before Renderer.hpp
#include <hppv/Renderer.hpp>
#include <hppv/App.hpp>
#include <hppv/Scene.hpp>
#include <hppv/Font.hpp>
#include <hppv/Framebuffer.hpp>

#include "Shaders.hpp"

namespace hppv
{

glm::vec2 Text::getSize() const
{
    auto x = 0.f;
    glm::vec2 size(0.f);

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

Renderer::Instance createInstance(glm::vec2 pos, glm::vec2 size, float rotation, glm::vec2 rotationPoint, glm::vec4 color,
                                  glm::vec4 texRect, const Texture& texture)
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

    auto texSize = texture.getSize();

    i.normTexRect.x = texRect.x / texSize.x;
    i.normTexRect.z = texRect.z / texSize.x;
    i.normTexRect.w = texRect.w / texSize.y;
    i.normTexRect.y = 1.f - texRect.y / texSize.y - i.normTexRect.w;

    return i;
}

Renderer::Renderer()
{
    glEnable(GL_BLEND);

    shaders_.emplace(Render::Color, Shader({vertexSource, colorSource}, "Render::Color"));
    shaders_.emplace(Render::Tex, Shader({vertexSource, texSource}, "Render::Tex"));
    shaders_.emplace(Render::TexPremultiplyAlpha, Shader({vertexSource, texPremultiplyAlphaSource}, "Render::TexPremultiplyAlpha"));
    shaders_.emplace(Render::CircleColor, Shader({vertexSource, circleColorSource}, "Render::CircleColor"));
    shaders_.emplace(Render::CircleTex, Shader({vertexSource, circleTexSource}, "Render::CircleTex"));
    shaders_.emplace(Render::CircleTexPremultiplyAlpha, Shader({vertexSource, circleTexPremultiplyAlphaSource},
                                                               "Render::CircleTexPremultiplyAlpha"));
    shaders_.emplace(Render::Sdf, Shader({vertexSource, sdfSource}, "Render::Sdf"));
    shaders_.emplace(Render::SdfOutline, Shader({vertexSource, sdfOutlineSource}, "Render::SdfOutline"));
    shaders_.emplace(Render::SdfGlow, Shader({vertexSource, sdfGlowSource}, "Render::SdfGlow"));
    shaders_.emplace(Render::SdfShadow, Shader({vertexSource, sdfShadowSource}, "Render::SdfShadow"));

    glSamplerParameteri(samplerLinear.getId(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(samplerNearest.getId(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    batches_.reserve(ReservedBatches);
    instances_.resize(ReservedInstances);
    texUnits_.reserve(ReservedTexUnits);
    uniforms_.reserve(ReservedUniforms);

    setTexUnitsDefault();

    batches_.emplace_back();
    {
        auto& first = batches_.front();
        first.scissorEnabled = false;
        first.shader = &shaders_.at(Render::Color);
        first.srcAlpha = GL_ONE;
        first.dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
        first.instances.start = 0;
        first.instances.count = 0;
        first.texUnits.start = 1; // first texUnit is omitted, it exists only for texUnits_.back().texture->getSize()
        first.texUnits.count = 0;
        first.uniforms.start = 0;
        first.uniforms.count = 0;
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

void Renderer::setScissor(glm::ivec4 scissor)
{
    scissor.y = App::getFrame().framebufferSize.y - scissor.y - scissor.w;
    auto& batch = getBatchToUpdate();
    batch.scissor = scissor;
    batch.scissorEnabled = true;
}

void Renderer::disableScissor()
{
    auto& batch = getBatchToUpdate();
    batch.scissorEnabled = false;
}

void Renderer::setViewport(glm::ivec4 viewport)
{
    viewport.y = App::getFrame().framebufferSize.y - viewport.y - viewport.w;
    auto& batch = getBatchToUpdate();
    batch.viewport = viewport;
}

void Renderer::setViewport(const Scene* scene)
{
    setViewport({scene->properties_.pos, scene->properties_.size});
}

void Renderer::setViewport(const Framebuffer& framebuffer)
{
    auto& batch = getBatchToUpdate();
    batch.viewport = {0, 0, framebuffer.getSize()};
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

void Renderer::setShader(Render mode)
{
    auto& batch = getBatchToUpdate();
    batch.shader = &shaders_.at(mode);
}

void Renderer::setTexture(Texture& texture, GLenum unit)
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

void Renderer::setSampler(GLsampler& sampler, GLenum unit)
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

void Renderer::setSampler(Sample mode, GLenum unit)
{
    setSampler(mode == Sample::Linear ? samplerLinear : samplerNearest, unit);
}

void Renderer::setBlend(GLenum srcAlpha, GLenum dstAlpha)
{
    auto& batch = getBatchToUpdate();
    batch.srcAlpha = srcAlpha;
    batch.dstAlpha = dstAlpha;
}

void Renderer::cache(const Sprite* sprite, std::size_t count)
{
    auto& batch = batches_.back();
    const auto start = batch.instances.start + batch.instances.count;
    batch.instances.count += count;
    const auto end = batch.instances.start + batch.instances.count;
    if(end > instances_.size())
        instances_.resize(end);

    for(auto i = start; i < end; ++i, ++sprite)
    {
        instances_[i] = createInstance(sprite->pos, sprite->size, sprite->rotation, sprite->rotationPoint,
                                       sprite->color, sprite->texRect, *texUnits_.back().texture);
    }
}

void Renderer::cache(const Circle* circle, std::size_t count)
{
    auto& batch = batches_.back();
    const auto start = batch.instances.start + batch.instances.count;
    batch.instances.count += count;
    const auto end = batch.instances.start + batch.instances.count;
    if(end > instances_.size())
        instances_.resize(end);

    for(auto i = start; i < end; ++i, ++circle)
    {
        instances_[i] = createInstance(circle->center - circle->radius, glm::vec2(circle->radius * 2.f), 0.f, {},
                                       circle->color, circle->texRect, *texUnits_.back().texture);
    }
}

void Renderer::cache(const Text& text)
{
    auto& batch = batches_.back();
    auto maxInstances = batch.instances.start + batch.instances.count + text.text.size();
    if(maxInstances > instances_.size())
        instances_.resize(maxInstances);

    auto penPos = text.pos;
    auto i = batch.instances.start + batch.instances.count;

    const auto halfTextSize = text.getSize() / 2.f;

    for(auto c: text.text)
    {
        if(c == '\n')
        {
            penPos.x = text.pos.x;
            penPos.y += text.font->getLineHeight() * text.scale;
            continue;
        }

        auto glyph = text.font->getGlyph(c);

        auto pos = penPos + glm::vec2(glyph.offset) * text.scale;
        auto size = glm::vec2(glyph.texRect.z, glyph.texRect.w) * text.scale;

        instances_[i] = createInstance(pos, size, text.rotation, text.rotationPoint + text.pos + halfTextSize - pos
                                       - size / 2.f, // this correction is needed, see createInstance
                                       text.color, glyph.texRect, *texUnits_.back().texture);

        penPos.x += glyph.advance * text.scale;
        ++i;
        ++batch.instances.count;
    }
}

void Renderer::flush()
{
    auto numInstances = batches_.back().instances.start + batches_.back().instances.count;

    glBindBuffer(GL_ARRAY_BUFFER, boInstances_.getId());

    glBufferData(GL_ARRAY_BUFFER, numInstances * sizeof(Instance), instances_.data(), GL_STREAM_DRAW);

    glBindVertexArray(vao_.getId());

    for(auto& batch: batches_)
    {
        if(batch.scissorEnabled)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(batch.scissor.x, batch.scissor.y, batch.scissor.z, batch.scissor.w);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }

        glViewport(batch.viewport.x, batch.viewport.y, batch.viewport.z, batch.viewport.w);

        batch.shader->bind();

        glUniformMatrix4fv(batch.shader->getUniformLocation("projection"), 1, GL_FALSE, &batch.projection[0][0]);

        {
            const auto start = batch.uniforms.start;
            const auto count = batch.uniforms.count;

            for(auto i = start; i < start + count; ++i)
                uniforms_[i].setter(batch.shader->getUniformLocation(uniforms_[i].name));
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

        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, batch.instances.count, batch.instances.start);
    }

    batches_.erase(batches_.begin(), batches_.end() - 1);
    uniforms_.clear();

    batches_.front().instances.start = 0;
    batches_.front().instances.count = 0;
    batches_.front().texUnits.start = 1;
    batches_.front().texUnits.count = 0;
    batches_.front().uniforms.start = 0;
    batches_.front().uniforms.count = 0;

    setTexUnitsDefault();
}

void Renderer::setTexUnitsDefault()
{
    texUnits_.clear();
    texUnits_.emplace_back();
    auto& first = texUnits_.front();
    first.unit = 0;
    first.texture = &texDummy;
    first.sampler = &samplerLinear;
}

Renderer::Batch& Renderer::getBatchToUpdate()
{
    {
        auto& current = batches_.back();
        if(!current.instances.count)
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

    return current;
}

} // namespace hppv
