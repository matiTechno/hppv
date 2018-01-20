#include <vector>
#include <thread>
#include <functional> // std::ref

#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/glad.h>

#include "renderImage.hpp"
#include "../run.hpp"

class RayTracer: public hppv::Scene
{
public:
    RayTracer()
    {
        properties_.maximize = true;

        std::thread t(renderImage, image_.buffer.data(), image_.size, std::ref(image_.renderProgress));
        t.detach();
    }

    void render(hppv::Renderer& renderer) override
    {
        // todo?: partial updates?
        if(!image_.ready && (image_.renderProgress == image_.size.x * image_.size.y))
        {
            image_.ready = true;

            GLint unpackAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // 3 is not supported

            image_.tex.bind();

            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_.size.x, image_.size.y, GL_RGB, GL_UNSIGNED_BYTE,
                            image_.buffer.data());

            glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
        }

        if(image_.ready)
        {
            const auto doesFit = image_.size.x <= properties_.size.x &&
                                 image_.size.y <= properties_.size.y;

            const auto proj = doesFit ? hppv::Space(0, 0, properties_.size) :
                                        hppv::expandToMatchAspectRatio(hppv::Space(0, 0, image_.size), properties_.size);

            renderer.projection(proj);
            renderer.shader(hppv::Render::Tex);
            renderer.texture(image_.tex);
            renderer.flipTextureY(true);

            hppv::Sprite sprite;
            sprite.size = image_.size;
            sprite.pos = proj.pos + (proj.size - sprite.size) / 2.f;
            sprite.texRect = {0, 0, image_.tex.getSize()};

            renderer.cache(sprite);
            renderer.flipTextureY(false);
        }
        else
        {
            const hppv::Space space(0, 0, 100, 100);

            renderer.projection(hppv::expandToMatchAspectRatio(space, properties_.size));
            renderer.shader(hppv::Render::Color);

            hppv::Sprite sprite;
            sprite.size = {50.f, 3.f};
            sprite.pos = space.pos + (space.size - sprite.size) / 2.f;
            sprite.color = {0.2f, 0.2f, 0.2f, 1.f};

            renderer.cache(sprite);

            sprite.size.x *= static_cast<float>(image_.renderProgress) / (image_.size.x * image_.size.y);
            sprite.color = {0.f, 1.f, 0.f, 1.f};

            renderer.cache(sprite);
        }
    }

private:
    struct Image
    {
        Image(): buffer(size.x * size.y), tex(GL_RGB8, size) {}

        static constexpr glm::ivec2 size{800, 600};
        std::vector<Pixel> buffer;
        hppv::Texture tex;
        std::atomic_int renderProgress = 0;
        bool ready = false;
    }
    image_;
};

RUN(RayTracer)
