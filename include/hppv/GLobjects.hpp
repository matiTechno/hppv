#pragma once

namespace hppv
{

using GLuint = unsigned int;

class GLobject
{
public:
    using Deleter = void(*)(GLuint);

    ~GLobject();
    GLobject(const GLobject&) = delete;
    GLobject& operator=(const GLobject&) = delete;
    GLobject(GLobject&& rhs);
    GLobject& operator=(GLobject&& rhs);

    GLuint getId() const {return id_;}

protected:
    GLuint id_ = 0;

    GLobject(Deleter deleter);

private:
    Deleter deleter_;
};


class GLvao: public GLobject
{
public:
    GLvao();
};

class GLbo: public GLobject
{
public:
    GLbo();
};

class GLtexture: public GLobject
{
public:
    GLtexture();
};

class GLsampler: public GLobject
{
public:
    GLsampler();
};

class GLframebuffer: public GLobject
{
public:
    GLframebuffer();
};

} // namespace hppv
