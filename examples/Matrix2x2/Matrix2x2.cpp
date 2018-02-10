#include <cassert>

#include <glm/common.hpp> // glm::mix
#include <glm/mat2x2.hpp>

#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

// inspired by 3Blue1Brown Essence of linear algebra
// todo: editor

struct M2
{
    glm::vec2 i{1.f, 0.f}, j{0.f, 1.f};
};

glm::vec2 operator*(const M2 m, const glm::vec2 v)
{
    return m.i * v.x + m.j * v.y;
}

M2 operator*(const M2 x, const M2 y)
{
    return {{x * y.i}, {x * y.j}};
}

void imguiPrintM2(const char* const name, const M2 m)
{
    if(name)
    {
        ImGui::Text("%s", name);
    }

    ImGui::Text("%.1f   %.1f\n\n"
                "%.1f   %.1f", m.i.x, m.j.x, m.i.y, m.j.y);
}

class Matrix2x2: public hppv::Scene
{
public:
    Matrix2x2()
    {
        properties_.maximize = true;

        for(auto i = 0; i < hppv::size(points_); ++i)
        {
            points_[i] = space_.pos + space_.size / static_cast<float>(rowSize_ - 1) *
                                      glm::vec2(i % rowSize_, i / rowSize_);
        }
    }

    void render(hppv::Renderer& renderer)
    {
        transition_.time += frame_.time;

        if(transition_.time > transition_.duration)
        {
            transition_.time = 0.f;
        }

        {
            constexpr auto offset = 1.f;
            renderer.projection(hppv::expandToMatchAspectRatio(hppv::Space(space_.pos - offset, space_.size + 2 * offset),
                                                               properties_.size));
        }

        // axes
        renderer.shader(hppv::Render::Color);

        {
            hppv::Sprite s;
            s.color = {0.2f, 0.2f, 0.2f, 1.f};
            constexpr auto width = 0.1f;

            // x
            s.size = {space_.size.x, width};
            s.pos = {space_.pos.x, 0.f - s.size.y / 2.f};
            renderer.cache(s);

            // y
            s.size = {width, space_.size.y};
            s.pos = {0.f - s.size.x / 2.f, space_.pos.y};
            renderer.cache(s);
        }

        renderer.shader(hppv::Render::CircleColor);

        constexpr M2 shear{{1.f, 0.f}, {1.f, 1.f}};
        constexpr M2 rotation90{{0.f, 1.f}, {-1.f, 0.f}};
        const auto outputMatrix = rotation90 * shear;

        assert(glm::mat2(rotation90.i, rotation90.j) * glm::mat2(shear.i, shear.j) * glm::vec2(1.f) ==
               outputMatrix * glm::vec2(1.f));

        for(const auto point: points_)
        {
            hppv::Circle c;
            glm::vec2 start, end;
            float a;

            if(sequentially_)
            {
                // note: constexpr failed to compile with clang 5.0.1
                const auto halfDuration = transition_.duration / 2.f;

                if(transition_.time < halfDuration)
                {
                    start = point;
                    end = shear * start;
                    a = transition_.time / halfDuration;
                }
                else
                {
                    start = shear * point;
                    end = rotation90 * start;
                    a = (transition_.time - halfDuration) / halfDuration;
                }
            }
            else
            {
                start = point;
                end = outputMatrix * start;
                a = transition_.time / transition_.duration;
            }

            c.center = glm::mix(start, end, a);
            c.center.y *= -1.f; // hack: in the Renderer coordinate system y grows down; we want the opposite
            c.radius = 0.065f;
            c.color = {1.f, 0.5f, 0.f, 1.f};
            renderer.cache(c);
        }

        ImGui::Begin("Matrix2x2");

        imguiPrintM2("rotation90", rotation90);
        ImGui::NewLine();
        ImGui::Text("X");
        ImGui::NewLine();
        imguiPrintM2("shear", shear);
        ImGui::NewLine();
        ImGui::Text("=");
        ImGui::NewLine();
        imguiPrintM2(nullptr, outputMatrix);
        ImGui::NewLine();

        if(ImGui::Checkbox("animate sequentially", &sequentially_))
        {
            transition_.time = 0.f;
        }

        ImGui::End();
    }

private:
    static constexpr auto rowSize_ = 11;
    const hppv::Space space_{-5.f, -5.f, 10.f, 10.f};
    glm::vec2 points_[rowSize_ * rowSize_];

    struct Transition
    {
        float time = 0.f;
        static constexpr auto duration = 10.f;
    }
    transition_;

    bool sequentially_ = true;
};

RUN(Matrix2x2)
