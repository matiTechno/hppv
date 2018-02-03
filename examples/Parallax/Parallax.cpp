#include <set>

#include <GLFW/glfw3.h>

#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/Texture.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

struct Layer
{
    hppv::Space space;
    hppv::Texture texture;
    float moveCoeff;
    hppv::Sprite sprite;
};

class Parallax: public hppv::Scene
{
public:
    Parallax()
    {
        properties_.maximize = true;

        layers_[0].texture = hppv::Texture("res/parallax-forest-back-trees.png");
        layers_[0].moveCoeff = 0.05f;

        layers_[1].texture = hppv::Texture("res/parallax-forest-lights.png");
        layers_[1].moveCoeff = 0.06f;

        layers_[2].texture = hppv::Texture("res/parallax-forest-middle-trees.png");
        layers_[2].moveCoeff = 0.3f;

        layers_[3].texture = hppv::Texture("res/parallax-forest-front-trees.png");
        layers_[3].moveCoeff = 1.f;

        for(auto& layer: layers_)
        {
            layer.space = hppv::Space(0, 0, layers_[0].texture.getSize());
            layer.sprite = hppv::Sprite(layer.space);
        }
    }

    void processInput(bool hasInput) override;
    void render(hppv::Renderer& renderer) override;

private:
    Layer layers_[4];
    std::set<int> keysHeld_;
    static inline float spaceVel_ = 200.f;
    static inline float zoomFactor_ = 1.1f;
    bool pilot_ = false;
};

void Parallax::processInput(const bool hasInput)
{
    std::set<int> keysPressed;

    for(const auto& event: frame_.events)
    {
        if(event.type == hppv::Event::Key)
        {
            if(event.key.action == GLFW_PRESS && hasInput)
            {
                keysPressed.insert(event.key.key);
                keysHeld_.insert(event.key.key);
            }
            else if(event.key.action == GLFW_RELEASE)
            {
                keysHeld_.erase(event.key.key);
            }
        }
    }

    keysPressed.insert(keysHeld_.begin(), keysHeld_.end());

    glm::vec2 velDir(0.f);

    if(keysPressed.count(GLFW_KEY_A) || keysPressed.count(GLFW_KEY_LEFT))
    {
        velDir.x -= 1.f;
    }
    if(keysPressed.count(GLFW_KEY_D) || keysPressed.count(GLFW_KEY_RIGHT))
    {
        velDir.x += 1.f;
    }
    if(keysPressed.count(GLFW_KEY_W) || keysPressed.count(GLFW_KEY_UP))
    {
        velDir.y -= 1.f;
    }
    if(keysPressed.count(GLFW_KEY_S) || keysPressed.count(GLFW_KEY_DOWN))
    {
        velDir.y += 1.f;
    }

    if(velDir != glm::vec2(0.f))
    {
        velDir = glm::normalize(velDir);
    }

    if(pilot_)
    {
        velDir = {1.f, 0.f};
    }

    float zoom = 1.f;

    if(keysPressed.count(GLFW_KEY_K))
    {
        zoom *= zoomFactor_;
    }

    if(keysPressed.count(GLFW_KEY_J))
    {
        zoom /= zoomFactor_;
    }

    for(auto& layer: layers_)
    {
        layer.space.pos.x += layer.moveCoeff * velDir.x * spaceVel_ * frame_.time;
        layer.space.pos.y += velDir.y * spaceVel_ * frame_.time;
        layer.space = hppv::zoomToCenter(layer.space, zoom);
    }
}

void Parallax::render(hppv::Renderer& renderer)
{
    ImGui::Begin("controls");
    ImGui::Text("wsad / arrows   move");
    ImGui::Text("kj              zoom");
    ImGui::Checkbox("pilot", &pilot_);
    ImGui::End();

    renderer.sampler(hppv::Sample::Nearest); // pixel art
    renderer.shader(hppv::Render::Tex);
    renderer.premultiplyAlpha(true);

    for(auto& layer: layers_)
    {
        const auto projection = hppv::expandToMatchAspectRatio(layer.space, properties_.size);

        renderer.projection(projection);
        renderer.texture(layer.texture);

        const auto d = projection.pos.x - layer.sprite.pos.x;
        float x = projection.pos.x;

        if(d < 0.f)
        {
            x = layer.sprite.pos.x - (static_cast<int>(glm::abs(d) / layer.sprite.size.x) + 1)
                                   * layer.sprite.size.x;
        }
        else if(d > 0.f)
        {
            x = layer.sprite.pos.x + static_cast<int>(glm::abs(d) / layer.sprite.size.x)
                                   * layer.sprite.size.x;
        }

        while(x < projection.pos.x + projection.size.x)
        {
            auto sprite = layer.sprite;
            sprite.pos.x = x;
            renderer.cache(sprite);
            x += sprite.size.x;
        }
    }
}

RUN(Parallax)
