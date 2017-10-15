#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include "Shader.hpp"
class Space;

struct Sprite
{
    glm::vec2 pos; // top left corner; y grows down
    glm::vec2 size;
    glm::vec4 color = {1.f, 1.f, 1.f, 1.f};
    float rotation = 0.f;
    glm::vec2 rotationPoint = {0.f, 0.f}; // distance from the sprite center
};

class Renderer
{
public:
    using GLuint = unsigned int;

    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // asserts that the cache is empty
    void setProjection(const Space& space);
    void setViewport(glm::ivec2 pos, glm::ivec2 size, glm::ivec2 framebufferSize);

    void cache(const Sprite& sprite);

    void cache(glm::vec2 pos, glm::vec2 size, glm::vec4 color, float rotation,
                glm::vec2 rotationPoint);

    // returns a number of rendered sprites
    int flush();

private:
    sh::Shader shader_;
    GLuint vao_, boQuad_, boInstances_;

    struct Instance
    {
        glm::vec4 color;
        glm::mat4 matrix;
    };

    std::vector<Instance> instances_;
};
