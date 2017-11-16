#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>

class GeometryLight: public hppv::PrototypeScene
{
public:
    GeometryLight(): hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {}

private:
    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.mode(hppv::Mode::Vertices);
        renderer.shader(hppv::Render::VerticesColor);

        using hppv::Vertex;

        Vertex v1;
        v1.pos = {30.f, 30.f};
        v1.color = {1.f, 0.f, 0.f, 1.f};
        Vertex v2;
        v2.pos = {60.f, 30.f};
        v2.color = {0.f, 1.f, 0.f, 1.f};
        Vertex v3;
        v3.pos = {50.f, 50.f};
        v3.color = {0.f, 0.f, 1.f, 1.f};

        renderer.cache(v1);
        renderer.cache(v2);
        renderer.cache(v3);
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
