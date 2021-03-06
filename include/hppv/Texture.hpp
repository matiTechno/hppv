#pragma once

#include <string>

#include <glm/vec2.hpp>

#include "GLobjects.hpp"

using GLenum = unsigned int;

namespace hppv
{

class Texture
{
public:
    explicit Texture(const std::string& filename);
    Texture(GLenum format, glm::ivec2 size);
    Texture();

    glm::ivec2 getSize() const {return size_;}

    void bind(GLuint unit = 0);

    GLuint getId() {return texture_.getId();}

private:
    GLtexture texture_;
    glm::ivec2 size_;

    void createDefault();
};

} // namespace hppv
