#pragma once

#include <glm/vec2.hpp>

#include "Texture.hpp"
#include "GLobjects.hpp"

namespace hppv
{

class Scene;

class Framebuffer
{
public:
    void bind();

    void unbind(); // binds default framebuffer

    Texture& getTexture() {return texture_;}

    glm::ivec2 getSize() const {return texture_.getSize();}

    // call bind before these

    void setSize(glm::ivec2 size);

    void clear();

private:
    GLframebuffer framebuffer_;
    Texture texture_;
};

} // namespace hppv
