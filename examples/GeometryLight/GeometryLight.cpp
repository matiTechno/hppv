#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>

class GeometryLight: public hppv::PrototypeScene
{
public:
    GeometryLight(): hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {}

private:
    void prototypeRender(hppv::Renderer& renderer) override
    {
        using hppv::Vertex;

        renderer.mode(hppv::Mode::Vertices);
        renderer.primitive(GL_LINE_LOOP);
        renderer.shader(hppv::Render::VerticesColor);

        {
            Vertex v1;
            v1.pos = {30.f, 30.f};
            Vertex v2;
            v2.pos = {60.f, 40.f};
            Vertex v3;
            v3.pos = {50.f, 20.f};

            renderer.cache(v1);
            renderer.cache(v2);
            renderer.cache(v3);
            renderer.breakBatch();
        }
        {
            Vertex v1;
            v1.pos = {10.f, 10.f};
            Vertex v2;
            v2.pos = {90.f, 10.f};
            Vertex v3;
            v3.pos = {90.f, 90.f};
            Vertex v4;
            v4.pos = {10.f, 90.f};

            renderer.cache(v1);
            renderer.cache(v2);
            renderer.cache(v3);
            renderer.cache(v4);
        }

        renderer.mode(hppv::Mode::Instances);
        renderer.shader(hppv::Render::CircleColor);

        {
            hppv::Circle circle;
            circle.color = {1.f, 0.f, 0.f, 1.f};
            circle.center = {50.f, 50.f};
            circle.radius = 1.f;

            renderer.cache(circle);
        }
    }
};

int main()
{
    hppv::App app;
    if(!app.initialize(false)) return -1;
    app.pushScene<GeometryLight>();
    app.run();
    return 0;
}
