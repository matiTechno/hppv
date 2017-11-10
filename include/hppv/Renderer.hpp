#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Space.hpp"
#include "GLobjects.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

namespace hppv
{

class Font;
class Scene;
class Framebuffer;

struct Text
{
    Text(const Font* font): font(font) {}
    Text(const Font& font): font(&font) {}
    glm::vec2 pos; // top left corner
    float scale = 1.f;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f; // angle in radians
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the Text center
    const Font* font;
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

    glm::vec2 pos;
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
    Tex,
    TexPremultiplyAlpha,
    CircleColor,
    CircleTex,
    CircleTexPremultiplyAlpha,
    Sdf,
    SdfOutline, // vec4 outlineColor; float outlineWidth [0, 0.5]
    SdfGlow, // vec4 glowColor; float glowWidth [0, 0.5]
    SdfShadow // vec4 shadowColor; float shadowSmoothing [0, 0.5]; vec2 shadowOffset ((1, 1) - offset by texture size)
};

enum class Sample
{
    Linear,
    Nearest
};

// in all cases x-axis grows right, y-axis grows down;
// state changes on non-empty cache will break batch unless
// it doesn't hold any instances

class Renderer
{
public:
    using GLint = int;

    Renderer();

    void setScissor(glm::ivec4 scissor);
    void disableScissor(); // on default

    void setViewport(glm::ivec4 viewport);
    void setViewport(const Scene* scene);
    void setViewport(const Framebuffer& framebuffer); // glViewport(0, 0, sizeX, sizeY)

    void setProjection(Space projection);

    void setShader(Shader& shader); // default is Render::Color
    void setShader(Render mode);

    template<typename Setter> // void(GLint location)
    void setUniform(const std::string& name, Setter&& setter)
    {
        uniforms_.emplace_back(Uniform{name, std::forward<Setter>(setter)});
        ++getBatchToUpdate().uniforms.count;
    }

    // texture state is lost after flush()
    void setTexture(Texture& texture, GLenum unit = 0);

    void setSampler(GLsampler& sampler, GLenum unit = 0);
    void setSampler(Sample mode, GLenum unit = 0); // default is Sample::Linear

    void setBlend(GLenum srcAlpha, GLenum dstAlpha); // default is GL_ONE, GL_ONE_MINUS_SRC_ALPHA

    void cache(const Sprite& sprite) {cache(&sprite, 1);}
    void cache(const Circle& circle) {cache(&circle, 1);}
    void cache(const Text& text);
    void cache(const Sprite* sprite, std::size_t count);
    void cache(const Circle* circle, std::size_t count);

    void flush();

    static const char* vertexSource;

    //layout(location = 0) in vec4 vertex;
    //layout(location = 1) in vec4 color;
    //layout(location = 2) in vec4 normTexRect;
    //layout(location = 3) in mat4 matrix;
    //uniform mat4 projection;
    //out vec4 vColor;
    //out vec2 vTexCoords;
    //out vec2 vPosition; // [0, 1], used for circle shading

    // internal use
    struct Instance
    {
        glm::mat4 matrix;
        glm::vec4 color;
        glm::vec4 normTexRect;
    };

private:
    enum
    {
        ReservedBatches = 50,
        ReservedInstances = 100000,
        ReservedTexUnits = 50,
        ReservedUniforms = 50
    };

    GLvao vao_;
    GLbo boQuad_;
    GLbo boInstances_;
    std::map<Render, Shader> shaders_;
    Texture texDummy;
    GLsampler samplerLinear;
    GLsampler samplerNearest;

    struct TexUnit
    {
        GLenum unit;
        Texture* texture;
        GLsampler* sampler;
    };

    struct Uniform
    {
        std::string name;
        std::function<void(GLint)> setter;
    };

    // viewport and scissor have y-axis in opengl coordinate system
    struct Batch
    {
        glm::ivec4 scissor;
        glm::ivec4 viewport;
        bool scissorEnabled;
        glm::mat4 projection;
        Shader* shader;
        GLenum srcAlpha;
        GLenum dstAlpha;
        struct
        {
            std::size_t start;
            std::size_t count;
        }
        instances, texUnits, uniforms;
    };

    std::vector<Batch> batches_;
    std::vector<Instance> instances_;
    std::vector<TexUnit> texUnits_;
    std::vector<Uniform> uniforms_;

    void setTexUnitsDefault();
    Batch& getBatchToUpdate();
};

} // namespace hppv
