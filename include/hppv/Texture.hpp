#pragma once

#include <string>

#include <glm/vec2.hpp>

#include "GLobjects.hpp"

namespace hppv
{

using GLenum = unsigned int;
using GLuint = unsigned int;

class Texture
{
public:
    Texture();
    explicit Texture(const std::string& filename);
    Texture(GLenum format, glm::ivec2 size);

    glm::ivec2 getSize() const {return size_;}

    void bind(GLuint unit = 0);

    GLuint getId() {return texture_.getId();}

private:
    GLtexture texture_;
    glm::ivec2 size_;

    void createDefault();
};

} // namespace hppv
