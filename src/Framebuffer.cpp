#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>

namespace hppv
{

Framebuffer::Framebuffer(GLenum textureFormat, int numAttachments):
    textureFormat_(textureFormat)
{
    bind();

    std::vector<GLenum> attachments(numAttachments);

    for(int i = 0; i < numAttachments; ++i)
    {
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glDrawBuffers(numAttachments, attachments.data());

    unbind();

    textures_.resize(numAttachments);
}

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
    for(std::size_t i = 0; i < textures_.size(); ++i)
    {
        textures_[i] = Texture(textureFormat_, size);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                               textures_[i].getId(), 0);
    }
}

void Framebuffer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

} // namespace hppv
