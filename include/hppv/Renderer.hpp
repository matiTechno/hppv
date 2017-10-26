#pragma once

#include <vector>
#include <map>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Rect.hpp"
#include "GLobjects.hpp"
#include "Shader.hpp"
#include "Font.hpp"

namespace hppv
{

class Texture;
class Scene;

struct Text
{
    glm::vec2 pos; // top left corner;
    float scale = 1.f;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the text center
    Font* font;
    std::string text;

    glm::vec2 getSize() const;
};

struct Sprite
{
    Sprite() = default;

    Sprite(const Text& text):
        pos(text.pos),
        size(text.getSize()),
        rotation(text.rotation),
        rotationPoint(text.rotationPoint)
    {}

    glm::vec2 pos; // top left corner;
    glm::vec2 size;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f};
    Rect texRect;
};


struct Circle
{
    glm::vec2 center;
    float radius;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    Rect texRect;
};

enum class Render
{
    Color,
    Texture,
    TextureFlippedY, // use when rendering to framebuffer
    CircleColor,
    CircleTexture,
    Font
};

enum class Sample
{
    Linear,
    Nearest
};

class Renderer
{
public:
    using GLenum = unsigned int;

    Renderer();

    // -----

    void setViewport(Rect viewport, glm::ivec2 framebufferSize);

    void setViewport(const Scene& scene);

    void setViewport(glm::ivec2 framebufferSize);

    // -----

    void setProjection(Rect projection); // y grows down

    // -----

    void setShader(Shader& shader);

    void setShader(Render mode);

    // -----

    void setTexture(Texture& texture, GLenum unit = 0);

    void setTexture(Font& font);

    // -----

    void setSampler(GLsampler& sampler);

    void setSampler(Sample mode);

    // -----

    // default is GL_ONE, GL_ONE_MINUS_SRC_ALPHA
    void setBlend(GLenum srcAlpha, GLenum dstAlpha);

    // -----

    void cache(const Sprite& sprite);

    void cache(const Circle& circle);

    void cache(const Text& text);

    void cache(const Sprite* sprites, int count);

    void cache(const Circle* circles, int count);

    void cache(const Text* texts, int count);

    // -----

    void flush();

    // -----

    Font* getFont() {return &font_;}

    // input:
    // layout(location = 0) in vec4 vertex; // pos + texCoords
    // layout(location = 1) in vec4 color;
    // layout(location = 2) in vec4 texRect;
    // layout(location = 3) in mat4 matrix;
    // uniform mat4 projection;
    //
    // output:
    // out vec4 vColor;
    // out vec2 vTexCoord;
    // out vec2 vPos; // range: 0 - 1; used for circle shading

    static const char* vertexShaderSource;
    static const char* vertexShaderFlippedYSource;

private:
    GLvao vao_;
    GLbo boQuad_;
    GLbo boInstances_;
    std::map<Render, Shader> shaders_;
    GLsampler samplerLinear;
    GLsampler samplerNearest;
    Font font_;

    struct Instance
    {
        glm::mat4 matrix;
        glm::vec4 color;
        glm::vec4 texRect;
    };

    struct Batch
    {
        Rect viewport;
        Rect projection;
        Shader* shader;
        Texture* texture;
        GLsampler* sampler;
        GLenum srcAlpha;
        GLenum dstAlpha;
        int start;
        int count;
    };

    std::vector<Instance> instances_;
    std::vector<Batch> batches_;

    Batch& getBatchToUpdate();
};

} // namespace hppv
