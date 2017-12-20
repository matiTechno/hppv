#include <string>

#include <GLFW/glfw3.h>

#include <hppv/Renderer.hpp>
#include <hppv/App.hpp>

#include "Menu.hpp"
#include "Snake.hpp"
#include "Background.hpp"

Menu::Menu(const int score, const Game state):
    score_(score)
{
    properties_.opaque = false;
    properties_.size = {600, 600};

    if(state == Game::Paused)
    {
        options_.push_back({"resume", [this]{properties_.numScenesToPop = 1;}});
    }
    else
    {
        options_.push_back({"play again", [this]{properties_.numScenesToPop = 2;
                                                 properties_.sceneToPush = std::make_unique<Snake>();}});
    }

    options_.push_back({"quit", []{hppv::App::request(hppv::Request::Quit);}});

    selectedOption_ = options_.begin();
}

void::Menu::processInput(const bool hasInput)
{
    properties_.pos = (frame_.framebufferSize - properties_.size) / 2;

    if(hasInput == false)
        return;

    for(const auto& event: frame_.events)
    {
        if(event.type == hppv::Event::Key && event.key.action == GLFW_PRESS)
        {
            switch(event.key.key)
            {
            case GLFW_KEY_ESCAPE: properties_.numScenesToPop = 1; return;

            case GLFW_KEY_S:
            case GLFW_KEY_DOWN:

                ++selectedOption_;
                animation_.reset();

                if(selectedOption_ == options_.end())
                {
                    selectedOption_ = options_.begin();
                }
                break;

            case GLFW_KEY_W:
            case GLFW_KEY_UP:

                --selectedOption_;
                animation_.reset();

                if(selectedOption_ == options_.begin() - 1)
                {
                    selectedOption_ = options_.end() - 1;
                }
                break;

            case GLFW_KEY_SPACE:
            case GLFW_KEY_ENTER: selectedOption_->action(); return;
            }
        }
    }
}

void Menu::render(hppv::Renderer& renderer)
{
    animation_.accumulator += frame_.time;

    while(animation_.accumulator >= animation_.timestep)
    {
        animation_.accumulator -= animation_.timestep;
        animation_.visible = !animation_.visible;
    }

    renderer.projection({0, 0, properties_.size});

    {
        renderer.shader(hppv::Render::Color);

        hppv::Sprite sprite;
        sprite.pos = {0.f, 0.f};
        sprite.size = properties_.size;
        sprite.color = {0.f, 0.f, 0.f, 0.7f};

        renderer.cache(sprite);
    }

    {
        auto& font = Background::resources->font;

        renderer.texture(font.getTexture());
        renderer.shader(hppv::Render::Font);

        glm::vec2 pos(20.f);

        {
            hppv::Text text(font);
            text.pos = pos;
            text.text = "score: " + std::to_string(score_);
            text.color = {1.f, 0.f, 1.f, 1.f};

            renderer.cache(text);
        }

        pos.y += font.getLineHeight() * 2.f;

        for(auto option = options_.cbegin(); option != options_.cend(); ++option)
        {
            hppv::Text text(font);
            text.pos = pos;
            text.text = option->name;

            if(selectedOption_ == option && animation_.visible)
            {
                auto text2 = text;
                text2.pos += glm::vec2(5.f, 2.f);
                text2.color = {1.f, 0.f, 0.f, 1.f};

                renderer.cache(text2);
            }

            renderer.cache(text);

            pos.y += font.getLineHeight();
        }
    }
}
