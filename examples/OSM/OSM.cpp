// wiki.openstreetmap.org

#include <fstream>
#include <vector>
#include <string> // std::string, std::stoul
#include <string_view>
#include <algorithm> // std::sort

#include <hppv/Prototype.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>

#include "../run.hpp"

// todo: Renderer support for projection with y-axis growing upwards

struct Node
{
    std::size_t id;
    glm::vec2 pos;
};

// todo: implement with std::lower_bound

template<typename It>
Node findNode(const It start, const It end, const std::size_t id)
{
    const auto mid = start + (end - start) / 2;

    if(mid->id == id)
    {
        return *mid;
    }
    else if(mid->id > id)
    {
        return findNode(start, mid - 1, id);
    }
    else
    {
        return findNode(mid + 1, end, id);
    }
}

class OSM: public hppv::Prototype
{
public:
    OSM():
        // hack
        hppv::Prototype({20.5f, /*51.8f*/ -52.7f, 1.12f, 1.f})
    {
        std::ifstream file("res/map.osm");

        std::vector<Node> nodes;

        std::string line;

        while(std::getline(file, line))
        {
            if(auto pos = line.find("<node id="); pos != std::string::npos)
            {
                pos = line.find('"', pos);
                ++pos;
                auto end = line.find('"', pos);

                Node node;
                node.id = std::stoul(line.substr(pos, end - pos));

                pos = line.find('"', end + 1);
                ++pos;
                end = line.find('"', pos);

                // hack
                node.pos.y = -std::stof(line.substr(pos, end - pos));

                pos = line.find('"', end + 1);
                ++pos;
                end = line.find('"', pos);

                node.pos.x = std::stof(line.substr(pos, end - pos));

                nodes.push_back(node);
            }
        }

        std::sort(nodes.begin(), nodes.end(), [](const Node& l, const Node& r){return l.id < r.id;});

        file.clear();
        file.seekg(0);

        std::vector<std::size_t> way;
        const std::string_view highway = "<tag k=\"highway\"";
        glm::vec4 color;

        while(std::getline(file, line))
        {
            if(auto pos = line.find("<nd ref="); pos != std::string::npos)
            {
                pos = line.find('"', pos);
                ++pos;
                const auto end = line.find('"', pos);
                way.push_back(std::stoul(line.substr(pos, end - pos)));
            }
            else if(auto pos = line.find(highway); pos != std::string::npos)
            {
                pos += highway.size();
                pos = line.find('"', pos);
                ++pos;
                const auto end = line.find('"', pos);

                if(const auto type = line.substr(pos, end - pos); type == "motorway" || type == "trunk" || type == "primary")
                {
                    color = {0.f, 1.f, 0.f, 0.f};
                }
                else
                {
                    color = {0.5f, 0.2f, 0.1f, 0.f};
                }
            }
            else if(line.find("</way>") != std::string::npos)
            {
                for(auto it = way.cbegin(); it < way.cend() - 1; ++it)
                {
                    for(auto i = 0; i < 2; ++i)
                    {
                        vertices_.push_back({findNode(nodes.begin(), nodes.end(), *(it + i)).pos, {}, color});
                    }
                }

                way.clear();
            }
        }
    }

private:
    std::vector<hppv::Vertex> vertices_;

    // todo: don't render the map data (big static data) with Renderer
    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.mode(hppv::RenderMode::Vertices);
        renderer.primitive(GL_LINES);
        renderer.shader(hppv::Render::VerticesColor);
        renderer.cache(vertices_.data(), vertices_.size());
    }
};

RUN(OSM)
