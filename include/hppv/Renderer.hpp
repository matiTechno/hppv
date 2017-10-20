#pragma once

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Shader.hpp"
#include "GLobjects.hpp"

namespace hppv
{

class Texture;
class Space;
class Scene;
class Font;

struct Sprite
{
    glm::vec2 pos; // top left corner; y grows down
    glm::vec2 size;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the sprite center
    glm::ivec4 texCoords;
};

struct Text
{
    float scale = 1.f;
    glm::vec2 pos;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    std::string text;
    Font* font;
};

// add support for array of sprites to avoid some if statements
// same for text
class Renderer
{
public:
    using GLuint = unsigned int;

    Renderer();

    void setViewport(glm::ivec2 pos, glm::ivec2 size, glm::ivec2 framebufferSize);

    void setProjection(const Space& space);

    void setShader(sh::Shader* shader); // nullptr to set default shader

    // todo: add unit parameter
    void setTexture(Texture* texture); // nullptr to disable texture sampling
                                       // if default shader is used

    void cache(const Sprite& sprite);

    void cache(const Text& text);

    int flush(); // returns a number of rendered instances

    static const char* vertexShaderSource;

private:
    sh::Shader shaderColor_, shaderTexture_, shaderFont_;
    GLvao vao_;
    GLbo boQuad_, boInstances_;

    struct Instance
    {
        glm::mat4 matrix;
        glm::vec4 color;
        glm::vec4 texCoords;
    };

    struct Batch
    {
        glm::ivec4 viewportCoords;
        glm::mat4 projection;
        sh::Shader* shader;
        Texture* texture;
        int start;
        int count;
    };

    std::vector<Instance> instances_;
    std::vector<Batch> batches_;

    Batch& getTargetBatch();
};

} // namespace hppv
