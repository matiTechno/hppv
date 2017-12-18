#pragma once

#include <string>
#include <vector>
#include <functional>

#include <hppv/Scene.hpp>

namespace hppv
{
class Font;
}

enum class Game
{
    Lost,
    Paused
};

class Menu: public hppv::Scene
{
public:
    Menu(int score, Game state);

    void processInput(bool hasInput) override;

    void render(hppv::Renderer& renderer) override;

private:
    const int score_;

    struct Animation
    {
        bool visible = true;
        float accumulator = 0.f;
        static inline float timeStep = 0.5f;
    }
    animation_;

    struct Option
    {
        std::string name;
        std::function<void(void)> action;
    };

    std::vector<Option> options_;
    std::vector<Option>::iterator selectedOption_;
};
