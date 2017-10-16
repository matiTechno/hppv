#include <iostream>

#include <hppv/Texture.hpp>
#include <hppv/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace hppv
{

Texture::Texture(const std::string& filename)
{
    bind();

    auto* data = stbi_load(filename.c_str(), &size_.x, &size_.y, nullptr, 4);

    if(!data)
    {
        std::cout << "stbi_load failed, filename = " << filename << std::endl;
        size_ = {100, 100};
    }

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size_.x, size_.y);

    if(data)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_.x, size_.y, GL_RGBA,
                        GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);
    }
}

Texture::Texture(GLtexture texture):
    texture_(std::move(texture))
{
    bind();

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &size_.x);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &size_.y);
}

Texture::Texture(GLenum format, glm::ivec2 size):
    size_(size)
{
    bind();

    glTexStorage2D(GL_TEXTURE_2D, 1, format, size.x, size.y);
}

void Texture::bind(GLenum unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_.getId());
}

} // namespace hppv
