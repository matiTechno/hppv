#pragma once

#include <cassert>

namespace hppv
{

using GLuint = unsigned int;

class GLobject
{
public:
    using Deleter = void(*)(GLuint);

    ~GLobject() {clean();}
    GLobject(const GLobject&) = delete;
    GLobject& operator=(const GLobject&) = delete;

    GLobject(GLobject&& rhs):
        id_(rhs.id_),
        deleter_(rhs.deleter_)
    {
        rhs.id_ = 0;
    }

    GLobject& operator=(GLobject&& rhs)
    {
        assert(this != &rhs);
        clean();
        id_ = rhs.id_;
        rhs.id_ = 0;
        return *this;
    }

    GLuint getId() const {return id_;}

protected:
    GLuint id_ = 0;

    GLobject(Deleter deleter): deleter_(deleter) {}

private:
    Deleter deleter_;

    void clean() {if(id_) deleter_(id_);}
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
