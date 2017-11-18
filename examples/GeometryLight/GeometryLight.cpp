#include <vector>
#include <optional>
#include <algorithm>
#include <cassert>

#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>

std::optional<glm::vec2> findRayEnd(glm::vec2 rayStart, float rayAngle, glm::vec2 segStart, glm::vec2 segEnd)
{
    // todo:
    // * case when the ray is parallel to y-axis
    // * remove assert
    assert(rayAngle > 0.f && rayAngle < glm::pi<float>() * 2.f);

    auto rayCoeff = glm::tan(rayAngle);

    float x;

    if(segStart.x == segEnd.x)
    {
        x = segStart.x;
    }
    else
    {
        auto segCoeff = (segEnd.y - segStart.y) / (segEnd.x - segStart.x);
        x = (segStart.y - segCoeff * segStart.x - rayStart.y + rayCoeff * rayStart.x) / (rayCoeff - segCoeff);
    }

    if(rayAngle > glm::pi<float>() / 2.f && rayAngle < 1.5f * glm::pi<float>())
    {
        if(x > rayStart.x)
            return {};
    }
    else
    {
        if(x < rayStart.x)
            return {};
    }

    if(x < std::min(segStart.x, segEnd.x) || x > std::max(segStart.x, segEnd.x))
        return {};

    return glm::vec2(x, rayCoeff * x + rayStart.y - rayCoeff * rayStart.x);
}

using hppv::Vertex;

class GeometryLight: public hppv::PrototypeScene
{
public:
    GeometryLight(): hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {
        {
            light_.center = {50.f, 50.f};
            light_.radius = 1.f;
            light_.color = {1.f, 0.f, 0.f, 1.f};
        }
        {
            lineLoops_.emplace_back();
            auto& loop = lineLoops_.back();

            Vertex v1;
            v1.pos = {30.f, 30.f};
            Vertex v2;
            v2.pos = {60.f, 40.f};
            Vertex v3;
            v3.pos = {50.f, 20.f};

            loop.push_back(v1);
            loop.push_back(v2);
            loop.push_back(v3);
        }
        {
            lineLoops_.emplace_back();
            auto& loop = lineLoops_.back();

            Vertex v1;
            v1.pos = {70.f, 15.f};
            Vertex v2;
            v2.pos = {60.f, 25.f};
            Vertex v3;
            v3.pos = {65.f, 30.f};

            loop.push_back(v1);
            loop.push_back(v2);
            loop.push_back(v3);
        }
        {
            lineLoops_.emplace_back();
            auto& loop = lineLoops_.back();

            Vertex v1;
            v1.pos = border_.pos;
            Vertex v2;
            v2.pos = border_.pos + glm::vec2(border_.size.x, 0.f);
            Vertex v3;
            v3.pos = border_.pos + border_.size;
            Vertex v4;
            v4.pos = border_.pos + glm::vec2(0.f, border_.size.y);

            loop.push_back(v1);
            loop.push_back(v2);
            loop.push_back(v3);
            loop.push_back(v4);
        }

        points_.push_back({65, 80});
    }

private:
    std::vector<std::vector<hppv::Vertex>> lineLoops_;
    hppv::Space border_{10.f, 10.f, 80.f, 80.f};
    hppv::Circle light_;
    std::vector<glm::vec2> points_;

    void setPoints()
    {
        points_.clear();

        constexpr int numRays = 50;

        for(auto angle = 0.01f; angle < glm::pi<float>() * 2.f; angle += glm::pi<float>() * 2.f / numRays)
        {
            glm::vec2 point(666.f);

            for(auto& loop: lineLoops_)
            {
                for(auto it = loop.begin(); it < loop.end(); ++it)
                {
                    auto segEnd = (it == loop.end() - 1 ? loop.begin()->pos : (it + 1)->pos);

                    if(auto newPoint = findRayEnd(light_.center, angle, it->pos, segEnd))
                    {
                        if(glm::length(*newPoint - light_.center) < glm::length(point - light_.center))
                        {
                            point = *newPoint;
                        }
                    }
                }
            }

            points_.push_back(point);
        }
    }

    void prototypeProcessInput(bool isInput) override
    {
       if(isInput)
       {
           auto pos = hppv::mapCursor(frame_.cursorPos, space_.projected, this);

           if(pos.x - light_.radius > border_.pos.x &&
              pos.y - light_.radius > border_.pos.y &&
              pos.x + light_.radius < border_.pos.x + border_.size.x &&
              pos.y + light_.radius < border_.pos.y + border_.size.y)
           {
               light_.center = pos;
           }
       }

       setPoints();
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        renderer.mode(hppv::Mode::Vertices);
        renderer.primitive(GL_LINE_LOOP);
        renderer.shader(hppv::Render::VerticesColor);

        for(auto& loop: lineLoops_)
        {
            for(auto& vertex: loop)
            {
                renderer.cache(vertex);
            }

            renderer.breakBatch();
        }

        for(auto& point: points_)
        {
            Vertex v;
            v.pos = point;
            v.color = {1.f, 0.f, 0.f, 1.f};
            renderer.cache(v);
            v.pos = light_.center;
            renderer.cache(v);
            renderer.breakBatch();
        }

        renderer.primitive(GL_TRIANGLES);

        for(auto it = points_.begin(); it < points_.end(); ++it)
        {
            auto last = (it == points_.end() - 1 ? *points_.begin() : *(it + 1));

            Vertex v;
            v.color = {0.5f, 0.f, 0.f, 0.5f};

            v.pos = light_.center;
            renderer.cache(v);
            v.pos = *it;
            renderer.cache(v);
            v.pos = last;
            renderer.cache(v);
        }

        renderer.mode(hppv::Mode::Instances);
        renderer.shader(hppv::Render::CircleColor);
        renderer.cache(light_);

        for(auto& point: points_)
        {
            hppv::Circle circle;
            circle.center = point;
            circle.radius = 0.5f;
            circle.color = light_.color;
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
