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
#include <hppv/Shader.hpp>
#include <hppv/Framebuffer.hpp>

static const char* blurSource = R"(

#fragment
#version 330

in vec2 vTexCoord;
in vec4 vColor;

uniform sampler2D sampler;

out vec4 color;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec4 sample = texture(sampler, vTexCoord) * weights[0];
    vec2 texOffset  = 1.0 / textureSize(sampler, 0);

    for(int i = 1; i < 5; ++i)
    {
        sample += texture(sampler, vTexCoord + vec2(texOffset.x * i, 0)) * weights[i];
        sample += texture(sampler, vTexCoord - vec2(texOffset.x * i, 0)) * weights[i];
    }

    for(int i = 1; i < 5; ++i)
    {
        sample += texture(sampler, vTexCoord + vec2(0, texOffset.y * i)) * weights[i];
        sample += texture(sampler, vTexCoord - vec2(0, texOffset.y * i)) * weights[i];
    }

    color = sample * vColor;
}
)";

static const char* lightSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;
in vec2 vPos;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPos - center);
    float alpha = 1 - smoothstep(0, radius, distanceFromCenter);
    vec4 sample = texture(sampler, vTexCoord);
    color = sample * vColor * alpha;
}
)";

std::optional<glm::vec2> findRayEnd(glm::vec2 rayStart, glm::vec2 rayDir, glm::vec2 segStart, glm::vec2 segEnd)
{
    // we don't need to handle this case
    // for each point we cast three rays with different coefficient
    if(rayDir.x == 0.f)
        return {};

    const auto rayCoeff = rayDir.y / rayDir.x;
    glm::vec2 rayEnd;

    if(segStart.x == segEnd.x)
    {
        auto x = segStart.x;
        rayEnd = {x, rayCoeff * x + rayStart.y - rayCoeff * rayStart.x};

        if(rayEnd.y < std::min(segStart.y, segEnd.y) || rayEnd.y > std::max(segStart.y, segEnd.y))
            return {};
    }
    else
    {
        const auto segCoeff = (segEnd.y - segStart.y) / (segEnd.x - segStart.x);

        if(rayCoeff == segCoeff)
            return {};

        auto x = (segStart.y - segCoeff * segStart.x - rayStart.y + rayCoeff * rayStart.x) / (rayCoeff - segCoeff);

        if(x < std::min(segStart.x, segEnd.x) || (x > std::max(segStart.x, segEnd.x)))
            return {};

        rayEnd = {x, rayCoeff * x + rayStart.y - rayCoeff * rayStart.x};
    }

    if(rayDir.x * (rayEnd.x - rayStart.x) < 0)
        return {};

    return rayEnd;
}

// todo: implement blur
class GeometryLight: public hppv::PrototypeScene
{
public:
    GeometryLight():
        hppv::PrototypeScene({0.f, 0.f, 100.f, 100.f}, 1.1f, false),
        shaderLight_({hppv::Renderer::vInstancesSource, lightSource}, "light"),
        shaderBlur_({hppv::Renderer::vInstancesSource, blurSource}, "blur"),
        fbBlur1_(GL_RGBA8, 1),
        fbBlur2_(GL_RGBA8, 1)
    {
        setLineLoops();
    }

private:
    std::vector<std::vector<glm::vec2>> lineLoops_;
    hppv::Space border_{10.f, 10.f, 80.f, 80.f};
    std::vector<glm::vec2> points_;
    glm::vec2 lightPos_;
    hppv::Shader shaderLight_, shaderBlur_;
    hppv::Framebuffer fbBlur1_, fbBlur2_;

    struct
    {
        bool drawPoints = true;
        bool drawLines = true;
        bool drawTriangles = true;
        bool release = true;
    }
    options_;

    void setLineLoops();

    void setPoints()
    {
        points_.clear();

        for(const auto& loop: lineLoops_)
        {
            for(auto point: loop)
            {
                for(int i = -1; i < 2; ++i)
                {
                    auto rayDir = glm::rotate(point - lightPos_, i * 0.00001f);
                    glm::vec2 rayEnd(1000.f);

                    if(i == 0)
                    {
                        rayEnd = point;
                    }

                    for(const auto& loop: lineLoops_)
                    {
                        for(auto it = loop.cbegin(); it < loop.cend(); ++it)
                        {
                            auto segEnd = (it == loop.end() - 1 ? *loop.begin() : *(it + 1));

                            if(auto newRayEnd = findRayEnd(lightPos_, rayDir, *it, segEnd))
                            {
                                if(glm::length(*newRayEnd - lightPos_) < glm::length(rayEnd - lightPos_))
                                {
                                    rayEnd = *newRayEnd;
                                }
                            }
                        }
                    }

                    points_.push_back(rayEnd);
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
            ImGui::Checkbox("release", &options_.release);
        }
        ImGui::End();


        if(options_.release)
        {
            fbBlur1_.bind();
            fbBlur1_.setSize(properties_.size);
            fbBlur1_.clear();
            renderer.viewport(fbBlur1_);
        }

        if(options_.drawTriangles || options_.release)
        {
            renderer.mode(hppv::Mode::Vertices);
            renderer.shader(hppv::Render::VerticesColor);
            renderer.primitive(GL_TRIANGLES);

            for(auto it = points_.cbegin(); it < points_.cend(); ++it)
            {
                auto last = (it == points_.end() - 1 ? *points_.begin() : *(it + 1));

                hppv::Vertex v;

                if(options_.release)
                {
                    v.color = {1.f, 1.f, 0.7f, 1.f};
                }
                else
                {
                    v.color = {0.3f, 0.f, 0.f, 1.f};
                }

                v.pos = lightPos_;
                renderer.cache(v);
                v.pos = *it;
                renderer.cache(v);
                v.pos = last;
                renderer.cache(v);
            }
        }

        if(options_.release)
        {
            renderer.flush();
            renderer.viewport(this);
            fbBlur1_.unbind();

            renderer.mode(hppv::Mode::Instances);
            renderer.shader(shaderLight_);
            renderer.texture(fbBlur1_.getTexture());

            hppv::Circle c;
            c.center = lightPos_;
            c.radius = border_.size.x / 1.4f;
            c.texRect = hppv::mapToFramebuffer(c.toVec4(), space_.projected, fbBlur1_);
            renderer.cache(c);
        }

        renderer.mode(hppv::Mode::Vertices);
        renderer.shader(hppv::Render::VerticesColor);
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

        if(options_.release)
            return;

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
