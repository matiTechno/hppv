#include <hppv/GLobjects.hpp>
#include <hppv/glad.h>

namespace hppv
{

GLvao::GLvao():
    GLobject([](GLuint id){glDeleteVertexArrays(1, &id);})
{
    glGenVertexArrays(1, &id_);
}

GLbo::GLbo():
    GLobject([](GLuint id){glDeleteBuffers(1, &id);})
{
    glGenBuffers(1, &id_);
}

GLtexture::GLtexture():
    GLobject([](GLuint id){glDeleteTextures(1, &id);})
{
    glGenTextures(1, &id_);
}

GLsampler::GLsampler():
    GLobject([](GLuint id){glDeleteSamplers(1, &id);})
{
    glGenSamplers(1, &id_);
}

GLframebuffer::GLframebuffer():
    GLobject([](GLuint id){glDeleteFramebuffers(1, &id);})
{
    glGenFramebuffers(1, &id_);
}

} // namespace hppv
