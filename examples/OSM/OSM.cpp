// wiki.openstreetmap.org

#include <fstream>
#include <vector>
#include <string>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>

// todo: Renderer support for projection with y-axis growing upwards

class OSM: public hppv::PrototypeScene
{
public:
    OSM():
        // hack
        hppv::PrototypeScene({20.5f, /*51.8f*/ -52.7f, 1.12f, 1.f}, 1.05f, true)
    {
        std::ifstream file("map.osm");

        std::string line;
        while(std::getline(file, line))
        {
            if(auto pos = line.find("<node"); pos != std::string::npos)
            {
                pos = line.find('"', pos);
                pos = line.find('"', pos + 1);
                pos = line.find('"', pos + 1);
                ++pos;
                auto end = line.find('"', pos);

                glm::vec2 point;
                // hack
                point.y = -std::stof(line.substr(pos, end - pos));

                pos = line.find('"', end + 1);
                ++pos;
                end = line.find('"', pos);

                point.x = std::stof(line.substr(pos, end - pos));

                vertices_.push_back({point, {}, glm::vec4(0.5f, 0.20f, 0.1f, 0.f)});
            }
        }
    }

private:
    std::vector<hppv::Vertex> vertices_;

    void prototypeRender(hppv::Renderer& renderer)
    {
        renderer.mode(hppv::RenderMode::Vertices);
        renderer.primitive(GL_POINTS);
        renderer.shader(hppv::Render::VerticesColor);
        renderer.cache(vertices_.data(), vertices_.size());
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return 1;
    app.pushScene<OSM>();
    app.run();
    return 0;
}
