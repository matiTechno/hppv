#pragma once

#include "Texture.hpp"
#include "GLobjects.hpp"

namespace hppv
{

class Framebuffer
{
public:
    enum {MaxAttachments = 5};

    Framebuffer(GLenum textureFormat, int numAttachments);

    void bind();

    void unbind(); // binds default framebuffer

    Texture& getTexture(int attachment = 0) {return textures_[attachment];}

    glm::ivec2 getSize() const {return textures_[0].getSize();}

    // call bind before these

    void setSize(glm::ivec2 size);

    void clear();

private:
    GLenum textureFormat_;
    int numAttachments_;
    GLframebuffer framebuffer_;
    Texture textures_[MaxAttachments];
};

} // namespace hppv
