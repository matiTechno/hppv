#include <stdlib.h>
#include <random>
#include <vector>
#include <assert.h>
#include <algorithm>

#include <GLFW/glfw3.h>

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

// some questions:
// which values to use for weights initialization, [0, 1] or [-1, 1] and why?
// how many hidden layers is best? how many hidden neurons per layer?
// why use an activation function?
// how to research these things? Can models be analyzed with hand written mathematics?

struct Net
{
    float* input_neurons;
    float* hidden_neurons;
    float* output_neurons;
    float* ih_weights; // input to hidden
    float* ho_weights; // hidden to output
    int input_size;
    int hidden_size;
    int output_size;
};

std::mt19937 _rng;

float get_random01()
{
    std::uniform_real_distribution<float> d(0.f, 1.f);
    return d(_rng);
}

Net net_create(int input_size, int hidden_size, int output_size)
{
    Net net;
    net.input_size = input_size;
    net.hidden_size = hidden_size;
    net.output_size = output_size;
    net.input_neurons = (float*)malloc(sizeof(float) * input_size);
    net.hidden_neurons = (float*)malloc(sizeof(float) * hidden_size);
    net.output_neurons = (float*)malloc(sizeof(float) * output_size);
    net.ih_weights = (float*)malloc(sizeof(float) * input_size * hidden_size);
    net.ho_weights = (float*)malloc(sizeof(float) * hidden_size * output_size);
    return net;
}

void net_free(Net& net)
{
    free(net.input_neurons);
    free(net.hidden_neurons);
    free(net.output_neurons);
    free(net.ih_weights);
    free(net.ho_weights);
}

void net_randomize_weights(Net& net)
{
    for(int i = 0; i < net.input_size * net.hidden_size; ++i)
        net.ih_weights[i] = get_random01() * 2.f - 1.f;

    for(int i = 0; i < net.hidden_size * net.output_size; ++i)
        net.ho_weights[i] = get_random01() * 2.f - 1.f;
}

void net_compute(Net& net)
{
    for(int i = 0; i < net.hidden_size; ++i)
    {
        net.hidden_neurons[i] = 0;

        for(int k = 0; k < net.input_size; ++k)
            net.hidden_neurons[i] += net.input_neurons[k] * net.ih_weights[k + net.input_size * i];
    }

    for(int i = 0; i < net.output_size; ++i)
    {
        net.output_neurons[i] = 0;

        for(int k = 0; k < net.hidden_size; ++k)
            net.output_neurons[i] += net.hidden_neurons[k] * net.ho_weights[k + net.hidden_size * i];
    }
}

struct Bird
{
    Net net;
    glm::vec2 pos;
    float vel;
    float radius;
    float score;
    bool dead;
};

struct Pipe
{
    glm::vec2 pos;
    glm::vec2 size;
    glm::vec4 color;
};

void update_pipes_life(hppv::Space space, std::vector<Pipe>& pipes)
{
    float xspacing = 65.f;
    float yspacing = 20.f;
    float pipe_sizex = 14.f;

    for(;;)
    {
        float posx;

        if(pipes.size())
            posx = pipes.back().pos.x + xspacing;
        else
            posx = space.pos.x + space.size.x / 2.f;

        if(posx > space.pos.x + space.size.x)
            break;

        Pipe pipe;
        pipe.pos = {posx, space.pos.y};
        pipe.size.x = pipe_sizex;
        pipe.size.y = (get_random01() * 0.6f + 0.2f) * space.size.y;
        pipes.push_back(pipe);

        pipe.pos.y = pipe.pos.y + pipe.size.y + yspacing;
        pipe.size.y = (space.pos.y + space.size.y) - pipe.pos.y;
        pipes.push_back(pipe);
    }

    for(;;)
    {
        float p = pipes.front().pos.x + pipes.front().size.x;

        if(p < space.pos.x)
            pipes.erase(pipes.begin());
        else
            break;
    }
}

class GeneticFlappy: public hppv::Prototype
{
public:
    GeneticFlappy():
        hppv::Prototype({0.f, 0.f, 100.f, 100.f})
    {
        std::random_device rd;
        _rng.seed(rd());
        birds.resize(100);

        for(Bird& bird: birds)
        {
            bird.net = net_create(5, 5, 1);
            net_randomize_weights(bird.net);
        }

        init_game();
    }

private:
    std::vector<Pipe> pipes;
    std::vector<Bird> birds;
    float vel_fly_up = -35.f;
    float acc_gravity = 50.f;
    float xvel = -30.f;
    int updates_per_frame = 1;
    bool play_as_human = false;

    void init_game()
    {
        for(Bird& bird: birds)
        {
            bird.pos = space_.projected.pos + glm::vec2(20.f);
            bird.vel = 0.f;
            bird.radius = 2.f;
            bird.score = 0.f;
            bird.dead = play_as_human;
        }
        pipes.clear();
        update_pipes_life(space_.projected, pipes);

        if(play_as_human)
            birds[0].dead = false;
    }

    void prototypeProcessInput(hppv::Pinput input) override
    {
        for(const hppv::Event& e: input.events)
        {
            if(play_as_human && birds[0].vel > 0.f && e.type == hppv::Event::Key && e.key.key == GLFW_KEY_W && e.key.action == GLFW_PRESS)
                birds[0].vel = vel_fly_up;

            if(e.type == hppv::Event::FramebufferSize)
                init_game();
        }
    }

    void update_sim()
    {
        for(Pipe& pipe: pipes)
            pipe.pos.x += xvel *frame_.time;

        update_pipes_life(space_.projected, pipes);

        float nearest_pipe_dist = 1000;
        Pipe nearest_pipe;
        Pipe nearest_pipe_bot;

        for(std::size_t i = 0; i < pipes.size(); i += 2) // skip top and bot pipes and once
        {
            float d = pipes[i].pos.x - birds[0].pos.x;

            if(d > -pipes[i].size.x && d < nearest_pipe_dist)
            {
                nearest_pipe_dist = d;
                nearest_pipe = pipes[i];
                nearest_pipe_bot = pipes[i + 1];
                pipes[i].color = {1.f, 0.f, 0.f, 1.f};
                pipes[i + 1].color = pipes[i].color;
            }
            else
            {
                pipes[i].color = glm::vec4(1.f);
                pipes[i + 1].color = pipes[i].color;
            }
        }

        for(Bird& bird: birds)
        {
            if(bird.dead)
                continue;

            bird.net.input_neurons[0] = nearest_pipe_dist / space_.current.size.x;
            bird.net.input_neurons[1] = (nearest_pipe.pos.y + nearest_pipe.size.y - space_.current.pos.y) / space_.current.size.y;
            bird.net.input_neurons[2] = (nearest_pipe_bot.pos.y - space_.current.pos.y) / space_.current.size.y;
            bird.net.input_neurons[3] = (bird.pos.y - space_.current.pos.y) / space_.current.size.y;
            bird.net.input_neurons[4] = bird.vel / 50.f;

            net_compute(bird.net);
            float fly = bird.net.output_neurons[0];

            if(fly > 0.5f && !play_as_human && bird.vel > 0.f)
                bird.vel = vel_fly_up;

            bird.pos.y += (bird.vel * frame_.time) + (0.5f * acc_gravity * frame_.time * frame_.time);
            bird.vel += acc_gravity * frame_.time;

            if(bird.pos.y - bird.radius < space_.current.pos.y || bird.pos.y + bird.radius > space_.current.pos.y + space_.current.size.y)
            {
                bird.dead = true;
                continue;
            }

            for(Pipe& pipe: pipes)
            {
                if(bird.pos.x > pipe.pos.x && bird.pos.x < pipe.pos.x + pipe.size.x && bird.pos.y > pipe.pos.y && bird.pos.y < pipe.pos.y + pipe.size.y)
                {
                    bird.dead = true;
                    continue;
                }
            }

            bird.score += frame_.time;
        }

        bool all_dead = true;

        for(Bird& bird: birds)
        {
            if(!bird.dead)
                all_dead = false;
        }

        if(all_dead)
        {
            // replicate best bird

            int best_idx = 0;
            int best_score = birds[0].score;

            for(int i = 1; i < birds.size(); ++i)
            {
                if(birds[i].score > best_score)
                {
                    best_score = birds[i].score;
                    best_idx = i;
                }
            }

            Bird& best_parent = birds[best_idx];

            for(Bird& bird: birds)
            {
                if(&bird == &best_parent)
                    continue;

                memcpy(bird.net.ih_weights, best_parent.net.ih_weights, sizeof(float) * bird.net.input_size * bird.net.hidden_size);
                memcpy(bird.net.ho_weights, best_parent.net.ho_weights, sizeof(float) * bird.net.hidden_size * bird.net.output_size);

                // apply random mutations

                for(int i = 0; i < bird.net.input_size * bird.net.hidden_size; ++i)
                {
                    if(get_random01() < 0.1f)
                        bird.net.ih_weights[i] += ((get_random01() * 2.f) - 1.f) / 3.f;
                }

                for(int i = 0; i < bird.net.hidden_size * bird.net.output_size; ++i)
                {
                    if(get_random01() < 0.1f)
                        bird.net.ho_weights[i] += ((get_random01() * 2.f) - 1.f) / 3.f;
                }
            }

            init_game();
        }
    }

    void prototypeRender(hppv::Renderer& rr) override
    {
        for(int i = 0; i < updates_per_frame; ++i)
            update_sim();

        if(updates_per_frame < 10) // I don't want to get an epilepsy
        {
            rr.shader(hppv::Render::Color);

            for(Pipe& pipe: pipes)
            {
                hppv::Sprite sprite;
                sprite.pos = pipe.pos;
                sprite.size = pipe.size;
                //sprite.color = pipe.color;
                rr.cache(sprite);
            }

            rr.shader(hppv::Render::CircleColor);

            for(Bird& bird: birds)
            {
                if(bird.dead)
                    continue;

                hppv::Circle circle;
                circle.center = bird.pos;
                circle.radius = 2.f;
                circle.color = {0.2f, 0.8f, 0.2f, 0.1f};
                rr.cache(circle);
            }
        }
        ImGui::Begin(prototype_.imguiWindowName);
        ImGui::InputInt("updates per frame", &updates_per_frame);
        int alive = 0;
        for(Bird& bird: birds)
            alive += !bird.dead;
        ImGui::Text("birds alive: %d", alive);

        float score;

        if(play_as_human)
            score = birds[0].score;
        else
        {
            for(Bird& bird: birds)
            {
                if(!bird.dead)
                {
                    score = bird.score;
                    break;
                }
            }
        }

        ImGui::Text("score: %f", score);

        if(ImGui::Checkbox("play as human", &play_as_human))
            init_game();

        ImGui::End();
    }
};

RUN(GeneticFlappy)
