#pragma once

#include <vector>
#include <functional>

#include <hppv/Scene.hpp>

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
        void reset()
        {
            accumulator = 0.f;
            visible = true;
        }

        bool visible = true;
        float accumulator = 0.f;
        static inline float timestep = 0.5f;
    }
    animation_;

    struct Option
    {
        const char* const name;
        std::function<void(void)> action;
    };

    std::vector<Option> options_;
    std::vector<Option>::iterator selectedOption_;
};
