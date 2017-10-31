#pragma once

#include <vector>
#include <string>
#include <map>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "GLobjects.hpp"
#include "Shader.hpp"
#include "Font.hpp"
#include "Space.hpp"
#include "Texture.hpp"

namespace hppv
{

class Scene;

struct Text
{
    glm::vec2 pos; // top left corner
    float scale = 1.f;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the center
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

    glm::vec2 pos; // top left corner
    glm::vec2 size;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f};
    glm::vec4 texRect;
};


struct Circle
{
    glm::vec2 center;
    float radius;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    glm::vec4 texRect;
};

enum class Render
{
    Color,
    Texture,
    TexturePremultiplyAlpha,
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

    void setScissor(glm::ivec4 scissor);

    void setScissor(const Scene& scene);

    // -----

    void setViewport(glm::ivec4 viewport);

    void setViewport(const Scene& scene);

    // -----

    void setProjection(Space projection); // y grows down

    // -----

    void setShader(Shader& shader);

    void setShader(Render mode);

    // -----

    void setTexture(Texture& texture, GLenum unit = 0);

    // -----

    void setSampler(GLsampler& sampler, GLenum unit = 0);

    void setSampler(Sample mode, GLenum unit = 0); // default is Sample::Linear

    // -----

    void setBlend(GLenum srcAlpha, GLenum dstAlpha); // default is GL_ONE, GL_ONE_MINUS_SRC_ALPHA

    // -----

    void cache(const Sprite& sprite);

    void cache(const Circle& circle);

    void cache(const Text& text);

    //void cache(const Sprite* sprites, int count);

    //void cache(const Circle* circles, int count);

    //void cache(const Text* texts, int count);

    // -----

    void flush();

    // input:
    //layout(location = 0) in vec4 vertex;
    //layout(location = 1) in vec4 color;
    //layout(location = 2) in vec4 normTexRect;
    //layout(location = 3) in mat4 matrix;
    //uniform mat4 projection;

    // output:
    //out vec4 vColor;
    //out vec2 vTexCoords;
    //out vec2 vPosition; // range: 0 - 1; used for circle shading

    static const char* vertexShaderSource;
    static const char* vertexShaderTextureFlippedYSource;

private:
    enum
    {
        ReservedInstances = 100000,
        ReservedTexUnits = 50,
        ReservedBatches = 50
    };

    GLvao vao_;
    GLbo boQuad_;
    GLbo boInstances_;
    std::map<Render, Shader> shaders_;
    GLsampler samplerLinear;
    GLsampler samplerNearest;
    Texture texDummy;

    struct Instance
    {
        glm::mat4 matrix;
        glm::vec4 color;
        glm::vec4 normTexRect;
    };

    struct TexUnit
    {
        int unit;
        Texture* texture;
        GLsampler* sampler;
    };

    struct Batch
    {
        glm::ivec4 viewport;
        glm::ivec4 scissor;
        glm::mat4 projection;
        Shader* shader;
        GLenum srcAlpha;
        GLenum dstAlpha;
        int startInstances;
        int countInstances;
        int startTexUnits;
        int countTexUnits;
    };

    std::vector<Instance> instances_;
    std::vector<TexUnit> texUnits_;
    std::vector<Batch> batches_;

    Batch& getBatchToUpdate();
};

} // namespace hppv
