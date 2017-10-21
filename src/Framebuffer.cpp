#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>

namespace hppv
{

void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.getId());
}

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::setSize(glm::ivec2 size)
{
    texture_ = Texture(GL_RGBA8, size);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           texture_.getId(), 0);
}

void Framebuffer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace hppv
