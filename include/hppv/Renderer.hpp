#pragma once

#include <vector>
#include <string>

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

struct Circle
{
    glm::vec2 center;
    float radius;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    glm::vec4 texRect;
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

    Sprite(const Circle& circle):
        pos(circle.center - circle.radius),
        size(circle.radius * 2.f)
    {}

    glm::vec2 pos = {0.f, 0.f};
    glm::vec2 size = {1.f, 1.f};
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f};
    glm::vec4 texRect;
};

enum class Render
{
    Color = 0,
    Tex = 1,
    CircleColor = 2,
    CircleTex = 3,
    Sdf = 4,
    SdfOutline = 5, // vec4 outlineColor; float outlineWidth [0, 0.5]
    SdfGlow = 6, // vec4 glowColor; float glowWidth [0, 0.5]
    SdfShadow = 7 // vec4 shadowColor; float shadowSmoothing [0, 0.5]; vec2 shadowOffset ((1, 1) - offset by texture size)
};

enum class Sample
{
    Linear,
    Nearest
};

// * in all cases x-axis grows right, y-axis grows down
// * state changes on non-empty cache will break batch unless
//   it doesn't hold any instances
// * texture / sampler / uniform states are lost after flush()

class Renderer
{
public:
    using GLint = int;

    Renderer();

    // ----- disabled on default

    void scissor(glm::ivec4 scissor);
    void disableScissor() {getBatchToUpdate().scissorEnabled = false;}

    // -----

    void viewport(glm::ivec4 viewport);
    void viewport(const Scene* scene);
    void viewport(const Framebuffer& framebuffer); // glViewport(0, 0, size.x, size.y)

    // ----- projected / visible area

    template<typename T>
    void projection(const T& projection) {getBatchToUpdate().projection = {projection.pos, projection.size};}

    void projection(Space projection) {getBatchToUpdate().projection = projection;} // for initializer list

    // ----- default is Render::Color

    void shader(Shader& shader) {getBatchToUpdate().shader = &shader;}
    void shader(Render mode);

    // -----

    void uniform(const std::string& name, int value);
    void uniform(const std::string& name, float value);
    void uniform(const std::string& name, glm::vec2 value);
    void uniform(const std::string& name, glm::vec3 value);
    void uniform(const std::string& name, glm::vec4 value);
    void uniform(const std::string& name, const glm::mat4& value);

    // ----- default sampler is Sample::Linear

    void texture(Texture& texture, GLenum unit = 0);
    void sampler(GLsampler& sampler, GLenum unit = 0);

    void sampler(Sample mode, GLenum unit = 0)
    {
        sampler(mode == Sample::Linear ? samplerLinear : samplerNearest, unit);
    }

    // ----- default is GL_ONE, GL_ONE_MINUS_SRC_ALPHA

    void blend(GLenum srcAlpha, GLenum dstAlpha)
    {
        auto& batch = getBatchToUpdate();
        batch.srcAlpha = srcAlpha;
        batch.dstAlpha = dstAlpha;
    }

    // ----- enabled on default

    void premultiplyAlpha(bool on) {getBatchToUpdate().premultiplyAlpha = on;}

    // ----- disabled on default

    void flipTexRectX(bool on) {getBatchToUpdate().flipTexRectX = on;}
    void flipTexRectY(bool on) {getBatchToUpdate().flipTexRectY = on;}
    void flipTextureY(bool on) {getBatchToUpdate().flipTextureY = on;}

    // -----

    void cache(const Sprite& sprite) {cache(&sprite, 1);}
    void cache(const Circle& circle) {cache(&circle, 1);}
    void cache(const Text& text);
    void cache(const Sprite* sprite, std::size_t count);
    void cache(const Circle* circle, std::size_t count);

    // -----

    void flush();

    // -----

    // out vec4 vColor;
    // out vec2 vTexCoords;
    // out vec2 vPosition; // [0, 1], used for circle shading and fwidth()

    static const char* vertexSource;

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
    Shader shaderBasic_, shaderSdf_;
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
        enum Type
        {
            I1,
            F1, F2, F3, F4,
            MAT4F
        }
        type;

        Uniform(Type type, const std::string& name):
            type(type),
            name(name)
        {}

        std::string name;

        union
        {
            int i1;
            float f1;
            glm::vec2 f2;
            glm::vec3 f3;
            glm::vec4 f4;
            glm::mat4 mat4f;
        };
    };

    // viewport and scissor have y-axis in opengl coordinate system
    struct Batch
    {
        glm::ivec4 scissor;
        glm::ivec4 viewport;
        bool scissorEnabled;
        Space projection;
        Shader* shader;
        GLenum srcAlpha;
        GLenum dstAlpha;
        bool premultiplyAlpha;
        bool flipTexRectX;
        bool flipTexRectY;
        bool flipTextureY;
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
