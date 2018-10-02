#include <GLFW/glfw3.h>

#include <hppv/Renderer.hpp>

#include "Snake.hpp"
#include "Menu.hpp"
#include "Background.hpp"

Snake::Snake()
{
    properties_.opaque = false;
    properties_.maximize = true;

    auto& rng = Background::resources->rng;

    std::uniform_int_distribution<int> dist(0, MapSize - 1);

    nodes_.push_back({dist(rng), dist(rng)});

    do
    {
        food_.pos = {dist(rng), dist(rng)};
    }
    while(food_.pos == nodes_.back());
}

void Snake::processInput(const std::vector<hppv::Event>& events)
{
    for(const auto& event: events)
    {
        if(event.type == hppv::Event::Key && event.key.action == GLFW_PRESS)
        {
            glm::ivec2 vel(0);

            switch(event.key.key)
            {
            case GLFW_KEY_ESCAPE:
                properties_.sceneToPush = std::make_unique<Menu>(nodes_.size() - 1, Game::Paused);
                break;

            case GLFW_KEY_S:
            case GLFW_KEY_DOWN: vel = {0, 1}; break;

            case GLFW_KEY_W:
            case GLFW_KEY_UP: vel = {0, -1}; break;

            case GLFW_KEY_A:
            case GLFW_KEY_LEFT: vel = {-1, 0}; break;

            case GLFW_KEY_D:
            case GLFW_KEY_RIGHT: vel = {1, 0}; break;
            }

            if((vel.x || vel.y) && vel != -vel_.current)
            {
                vel_.next = vel;
            }
        }
    }
}

void Snake::update()
{
    accumulator_ += frame_.time;

    while(accumulator_ >= timestep_)
    {
        accumulator_ -= timestep_;
        food_.timeLeft -= timestep_;

        // move the body
        for(auto it = nodes_.rbegin(); it != nodes_.rend() - 1; ++it)
        {
            *it = *(it + 1);
        }

        // move the head
        {
            vel_.current = vel_.next;

            auto& head = nodes_.front();

            head += vel_.current;

            if(head.x > MapSize - 1)
            {
                head.x = 0;
            }
            else if(head.x < 0)
            {
                head.x = MapSize - 1;
            }

            if(head.y > MapSize - 1)
            {
                head.y = 0;
            }
            else if(head.y < 0)
            {
                head.y = MapSize - 1;
            }
        }

        // attach a new node
        {
            for(auto& node: newNodes_)
            {
                --node.turnsToSpawn;
            }

            // a little hack for a smoother animation
            if(newNodes_.size() && newNodes_.front().turnsToSpawn == -1)
            {
                newNodes_.erase(newNodes_.begin());
            }

            if(newNodes_.size() && newNodes_.front().turnsToSpawn == 0)
            {
                nodes_.push_back(newNodes_.front().pos);
            }
        }

        // check for collisions
        {
            const auto head = nodes_.front();

            for(auto it = nodes_.cbegin() + 1; it != nodes_.cend(); ++it)
            {
                if(head == *it)
                {
                    properties_.sceneToPush = std::make_unique<Menu>(nodes_.size() - 1, Game::Lost);
                }
            }

            if(head == food_.pos)
            {
                food_.timeLeft = 0.f;
                newNodes_.push_back({food_.pos, static_cast<int>(nodes_.size())});
            }
        }

        // spawn the food
        if(food_.timeLeft <= 0.f)
        {
            food_.timeLeft = food_.spawnTime;

            enum class Tile {Free, Used};

            constexpr auto numTiles = MapSize * MapSize;

            Tile map[numTiles];

            for(auto& tile: map)
            {
                tile = Tile::Free;
            }

            for(const auto node: nodes_)
            {
                map[node.y * MapSize + node.x] = Tile::Used;
            }

            int freeTiles[numTiles];

            auto numFreeTiles = 0;

            for(auto i = 0; i < numTiles; ++i)
            {
                if(map[i] == Tile::Free)
                {
                    freeTiles[numFreeTiles] = i;
                    ++numFreeTiles;
                }
            }

            std::uniform_int_distribution<int> dist(0, numFreeTiles - 1);

            const auto tileId = freeTiles[dist(Background::resources->rng)];
            food_.pos = {tileId % MapSize, tileId / MapSize};
        }
    }
}

void Snake::render(hppv::Renderer& renderer)
{
    const hppv::Space mapSpace(0.f, 0.f, 100.f, 100.f);

    {
        const glm::vec2 mapOffset(2.f);
        const hppv::Space space(mapSpace.pos - mapOffset, mapSpace.size + 2.f * mapOffset);
        renderer.projection(hppv::expandToMatchAspectRatio(space, frame_.framebufferSize));
    }

    const auto tileSize = mapSpace.size / glm::vec2(MapSize);
    constexpr auto tileSpacing = 0.25f;

    // tiles
    renderer.shader(hppv::Render::Color);

    for(auto i = 0; i < MapSize; ++i)
    {
        for(auto j = 0; j < MapSize; ++j)
        {
            hppv::Sprite sprite;
            sprite.pos = glm::vec2(i, j) * tileSize + tileSpacing;
            sprite.size = tileSize - 2.f * tileSpacing;
            sprite.color = {0.f, 0.f, 0.f, 0.6f};

            renderer.cache(sprite);
        }
    }

    // food
    renderer.texture(Background::resources->texSnakeFood);
    renderer.shader(hppv::Render::Tex);

    {
        hppv::Sprite sprite;
        sprite.size = tileSize - 2.f * tileSpacing;

        for(const auto node: newNodes_)
        {
            sprite.pos = glm::vec2(node.pos) * tileSize + tileSpacing;

            renderer.cache(sprite);
        }

        sprite.pos = glm::vec2(food_.pos) * tileSize + tileSpacing;

        renderer.cache(sprite);
    }

    // snake
    renderer.shader(hppv::Render::Color);

    for(auto it = nodes_.cbegin(); it != nodes_.cend(); ++it)
    {
        hppv::Sprite sprite;
        sprite.pos = glm::vec2(*it) * tileSize + tileSpacing;
        sprite.size = tileSize - 2.f * tileSpacing;

        if(it == nodes_.begin())
        {
            sprite.color = {1.f, 0.65f, 0.f, 1.f};
        }
        else
        {
            sprite.color = {1.f, 0.f, 0.15f, 0.5f};
        }

        for(const auto node: newNodes_)
        {
            if(*it == node.pos)
            {
                sprite.color /= 2.f;
                sprite.color.a = 0.f;
                break;
            }
        }

        renderer.cache(sprite);
    }
}
