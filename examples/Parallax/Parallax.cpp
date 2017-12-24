#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>

#include "../run.hpp"

struct Layer
{
    hppv::Space space;
    hppv::Texture texture;
    float moveCoeff;
    hppv::Sprite sprite;
};

// todo: zooming

// Implementing this with Prototype doesn't seem right.

class Parallax: public hppv::Prototype
{
public:
    Parallax():
        hppv::Prototype({-100.f, -100.f, 400.f, 400.f}),
        pos_(space_.current.pos)
    {
        properties_.maximize = true;

        layers_[0].texture = hppv::Texture("res/far-buildings.png");
        layers_[0].moveCoeff = 0.1f;

        layers_[1].texture = hppv::Texture("res/back-buildings.png");
        layers_[1].moveCoeff = 0.5f;

        layers_[2].texture = hppv::Texture("res/foreground.png");
        layers_[2].moveCoeff = 1.f;

        for(auto& layer: layers_)
        {
            layer.space = space_.current;

            const auto size = layer.texture.getSize();
            layer.sprite.texRect = {0, 0, size};
            layer.sprite.pos = {0, 0};
            layer.sprite.size = size;
        }
    }

private:
    Layer layers_[3];
    glm::vec2 pos_;

    void prototypeRender(hppv::Renderer& renderer) override
    {
        const auto move = space_.current.pos - pos_;
        pos_ = space_.current.pos;

        renderer.sampler(hppv::Sample::Nearest); // pixel art
        renderer.shader(hppv::Render::Tex);

        for(auto& layer: layers_)
        {
            layer.space.pos += layer.moveCoeff * move;

            renderer.projection(hppv::expandToMatchAspectRatio(layer.space, properties_.size));
            renderer.texture(layer.texture);
            renderer.cache(layer.sprite);
        }
    }
};

RUN(Parallax)
