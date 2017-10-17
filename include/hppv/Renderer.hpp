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

struct Sprite
{
    glm::vec2 pos; // top left corner; y grows down
    glm::vec2 size;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the sprite center
    Texture* texture = nullptr;
    glm::ivec4 texCoords;
};

class Renderer
{
public:
    using GLuint = unsigned int;

    Renderer();

    void setProjection(const Space& space);

    void setViewport(glm::ivec2 framebufferSize);
    void setViewport(const Scene& scene);

    void cache(const Sprite& sprite);

    void cache(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotation,
               glm::vec2 rotationPoint, Texture* texture, glm::ivec4 texCoords);

    // returns a number of rendered sprites
    int flush();

private:
    sh::Shader shader_;
    GLvao vao_;
    GLbo boQuad_, boInstances_;

    struct Instance
    {
        glm::vec4 color;
        glm::vec4 texCoords;
        glm::mat4 matrix;
    };

    struct Batch
    {
        int type;
        Texture* texture;
        int start;
        int count;
    };

    std::vector<Instance> instances_;
    std::vector<Batch> batches_;
};

} // namespace hppv
