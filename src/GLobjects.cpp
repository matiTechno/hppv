#include <hppv/GLobjects.hpp>
#include <hppv/glad.h>

namespace hppv
{

GLobject::GLobject(Deleter deleter):
    deleter_(deleter)
{}

GLobject::~GLobject()
{
    if(id_)
        deleter_(id_);
}

GLobject::GLobject(GLobject&& rhs):
    id_(rhs.id_)
{
    rhs.id_ = 0;
}

GLobject& GLobject::operator=(GLobject&& rhs)
{
    if(this != &rhs)
    {
        this->~GLobject();
        id_ = rhs.id_;
        rhs.id_ = 0;
    }
    return *this;
}

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
