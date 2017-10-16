#pragma once

#include <string>

#include <glm/vec2.hpp>

#include "GLobjects.hpp"

namespace hppv
{

using GLenum = unsigned int;

class Texture
{
public:
    // all constructors call bind()
    Texture(const std::string& filename);
    Texture(GLtexture texture);
    Texture(GLenum format, glm::ivec2 size);

    glm::ivec2 getSize() const {return size_;}

    void bind(GLenum unit = 0);

private:
    GLtexture texture_;
    glm::ivec2 size_;
};

} // namespace hppv
