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

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filename.c_str(), &size_.x, &size_.y, nullptr, 4);

    if(!data)
    {
        std::cout << "Texture: stbi_load() failed, filename = " << filename << std::endl;
        createDefault();
        return;
    }

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size_.x, size_.y);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_.x, size_.y, GL_RGBA,
                    GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);
}

Texture::Texture(const GLenum format, const glm::ivec2 size):
    size_(size)
{
    bind();

    glTexStorage2D(GL_TEXTURE_2D, 1, format, size.x, size.y);
}

Texture::Texture()
{
    bind();
    createDefault();
}

void Texture::bind(const GLuint unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_.getId());
}

void Texture::createDefault()
{
    size_ = {1, 1};

    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, size_.x, size_.y);

    const unsigned char pixel[] = {0, 255, 0, 255};

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size_.x, size_.y, GL_RGBA,
                    GL_UNSIGNED_BYTE, pixel);
}

} // namespace hppv
