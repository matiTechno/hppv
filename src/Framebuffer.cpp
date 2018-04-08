#include <assert.h>

#include <hppv/Framebuffer.hpp>
#include <hppv/glad.h>

namespace hppv
{

Framebuffer::Framebuffer(const GLenum textureFormat, const int numAttachments):
    textureFormat_(textureFormat),
    numAttachments_(numAttachments)
{
    assert(numAttachments <= MaxAttachments);

    bind();

    GLenum attachments[MaxAttachments];

    for(auto i = 0; i < numAttachments; ++i)
    {
        attachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glDrawBuffers(numAttachments, attachments);

    unbind();
}

void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_.getId());
}

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::setSize(const glm::ivec2 size)
{
    for(auto i = 0; i < numAttachments_; ++i)
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
