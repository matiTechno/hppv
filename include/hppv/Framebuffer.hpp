#pragma once

#include <vector>

#include <glm/vec2.hpp>

#include "Texture.hpp"
#include "GLobjects.hpp"

namespace hppv
{

class Framebuffer
{
public:
    Framebuffer(GLenum textureFormat, int numAttachments);

    void bind();

    void unbind(); // binds default framebuffer

    Texture& getTexture(int attachment) {return textures_[attachment];}

    glm::ivec2 getSize() const {return textures_[0].getSize();}

    // call bind before these

    void setSize(glm::ivec2 size);

    void clear();

private:
    enum {MaxAttachments = 10};
    GLframebuffer framebuffer_;
    std::vector<Texture> textures_;
    GLenum textureFormat_;
};

} // namespace hppv
