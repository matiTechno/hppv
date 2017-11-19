#include <vector>
#include <optional>
#include <algorithm>
#include <cmath>

#include <glm/trigonometric.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <hppv/App.hpp>
#include <hppv/PrototypeScene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/glad.h>
#include <hppv/imgui.h>

std::optional<glm::vec2> findRayEnd(glm::vec2 rayStart, glm::vec2 rayPoint, glm::vec2 segStart, glm::vec2 segEnd)
{
    auto rayCoeff = (rayPoint.y - rayStart.y) / (rayPoint.x - rayStart.x);

    float x;

    if(segStart.x == segEnd.x)
    {
        x = segStart.x;
    }
    else
    {
        auto segCoeff = (segEnd.y - segStart.y) / (segEnd.x - segStart.x);

        if(segCoeff == rayCoeff)
            return {};

        x = (segStart.y - segCoeff * segStart.x - rayStart.y + rayCoeff * rayStart.x) / (rayCoeff - segCoeff);
    }

    if((rayPoint.x - rayStart.x) * (x - rayStart.x) < 0)
        return {};

    if(x < std::min(segStart.x, segEnd.x) || x > std::max(segStart.x, segEnd.x))
        return {};

    glm::vec2 rayEnd(x, rayCoeff * x + rayStart.y - rayCoeff * rayStart.x);

    if(rayEnd.y < std::min(segStart.y, segEnd.y) || rayEnd.y > std::max(segStart.y, segEnd.y))
        return {};

    return rayEnd;
}

class GeometryLight: public hppv::PrototypeScene
{
public:
    GeometryLight(): hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false)
    {
        setLineLoops();
    }

private:
    std::vector<std::vector<glm::vec2>> lineLoops_;
    hppv::Space border_{10.f, 10.f, 80.f, 80.f};
    std::vector<glm::vec2> points_;
    glm::vec2 lightPos_;

    struct
    {
        bool drawPoints = true;
        bool drawLines = true;
        bool drawTriangles = true;
    }
    options_;

    void setLineLoops();

    void setPoints()
    {
        points_.clear();

        for(const auto& loop: lineLoops_)
        {
            // change to: i = -1; i < 2
            for(int i = 0; i < 1; ++i)
            {
                for(auto point: loop)
                {
                    std::optional<glm::vec2> rayEnd;

                    for(const auto& loop: lineLoops_)
                    {
                        for(auto it = loop.cbegin(); it < loop.cend(); ++it)
                        {
                            auto segEnd = (it == loop.end() - 1 ? *loop.begin() : *(it + 1));
                            auto rayPoint = glm::rotate(point, i * 0.00001f);

                            if(auto newRayEnd = findRayEnd(lightPos_, rayPoint, *it, segEnd))
                            {
                                if(!rayEnd || glm::length(*newRayEnd - lightPos_) < glm::length(*rayEnd - lightPos_))
                                {
                                    rayEnd = *newRayEnd;
                                }
                            }
                        }
                    }

                    if(rayEnd)
                    {
                        points_.push_back(*rayEnd);
                    }
                }
            }
        }

        std::sort(points_.begin(), points_.end(), [lightPos = lightPos_](const glm::vec2& l, const glm::vec2& r)
        {
            return std::atan2(l.x - lightPos.x, l.y - lightPos.y) <
                   std::atan2(r.x - lightPos.x, r.y - lightPos.y);
        });
    }

    void prototypeProcessInput(bool isInput) override
    {
        if(isInput)
        {
            lightPos_ = hppv::mapCursor(frame_.cursorPos, space_.projected, this);
            lightPos_.x = std::max(lightPos_.x, border_.pos.x + 1);
            lightPos_.y = std::max(lightPos_.y, border_.pos.y + 1);
            lightPos_.x = std::min(lightPos_.x, border_.pos.x + border_.size.x - 1);
            lightPos_.y = std::min(lightPos_.y, border_.pos.y + border_.size.y - 1);
        }

        setPoints();
    }

    void prototypeRender(hppv::Renderer& renderer) override
    {
        ImGui::Begin("light options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        {
            ImGui::Checkbox("draw points", &options_.drawPoints);
            ImGui::Checkbox("draw lines", &options_.drawLines);
            ImGui::Checkbox("draw triangles", &options_.drawTriangles);
        }
        ImGui::End();

        renderer.mode(hppv::Mode::Vertices);
        renderer.shader(hppv::Render::VerticesColor);

        if(options_.drawTriangles)
        {
            renderer.primitive(GL_TRIANGLES);

            for(auto it = points_.cbegin(); it < points_.cend(); ++it)
            {
                auto last = (it == points_.end() - 1 ? *points_.begin() : *(it + 1));

                hppv::Vertex v;
                v.color = {0.3f, 0.f, 0.f, 0.5f};

                v.pos = lightPos_;
                renderer.cache(v);
                v.pos = *it;
                renderer.cache(v);
                v.pos = last;
                renderer.cache(v);
            }
        }

        renderer.primitive(GL_LINE_LOOP);

        for(const auto& loop: lineLoops_)
        {
            for(auto point: loop)
            {
                hppv::Vertex v;
                v.pos = point;
                v.color = {0.5f, 0.5f, 0.5f, 1.f};
                renderer.cache(v);
            }

            renderer.breakBatch();
        }

        if(options_.drawLines)
        {
            for(auto point: points_)
            {
                hppv::Vertex v;
                v.pos = point;
                v.color = {1.f, 0.f, 0.f, 1.f};
                renderer.cache(v);
                v.pos = lightPos_;
                renderer.cache(v);
                renderer.breakBatch();
            }
        }

        if(options_.drawPoints)
        {
            renderer.mode(hppv::Mode::Instances);
            renderer.shader(hppv::Render::CircleColor);

            for(auto point: points_)
            {
                hppv::Circle c;
                c.center = point;
                c.radius = 0.5f;
                c.color = {1.f, 0.f, 0.f, 1.f};
                renderer.cache(c);
            }
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

void GeometryLight::setLineLoops()
{
    {
        lineLoops_.emplace_back();
        auto& loop = lineLoops_.back();
        loop.emplace_back(30.f, 30.f);
        loop.emplace_back(60.f, 40.f);
        loop.emplace_back(50.f, 20.f);
    }
    {
        lineLoops_.emplace_back();
        auto& loop = lineLoops_.back();
        loop.emplace_back(70.f, 15.f);
        loop.emplace_back(60.f, 25.f);
        loop.emplace_back(65.f, 30.f);
    }
    {
        lineLoops_.emplace_back();
        auto& loop = lineLoops_.back();
        loop.emplace_back(20.f, 60.f);
        loop.emplace_back(40.f, 60.f);
        loop.emplace_back(40.f, 75.f);
        loop.emplace_back(30.f, 75.f);
    }
    {
        lineLoops_.emplace_back();
        auto& loop = lineLoops_.back();
        loop.emplace_back(65.f, 40.f);
        loop.emplace_back(78.f, 30.f);
        loop.emplace_back(70.f, 50.f);
        loop.emplace_back(65.f, 45.f);
    }
    {
        lineLoops_.emplace_back();
        auto& loop = lineLoops_.back();
        loop.emplace_back(border_.pos);
        loop.emplace_back(border_.pos + glm::vec2(border_.size.x, 0.f));
        loop.emplace_back(border_.pos + border_.size);
        loop.emplace_back(border_.pos + glm::vec2(0.f, border_.size.y));
    }
}
