#include <cassert>

#include <glm/common.hpp>
#include <glm/mat2x2.hpp>

#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

// inspired by 3Blue1Brown Essence of linear algebra

struct M2x2
{
    glm::vec2 i{1.f, 0.f}, j{0.f, 1.f};
};

glm::vec2 operator*(const M2x2 m, const glm::vec2 v)
{
    return m.i * v.x + m.j * v.y;
}

// todo: transformations editor

class Matrix2x2: public hppv::Scene
{
public:
    Matrix2x2()
    {
        properties_.maximize = true;

        for(auto i = 0; i < hppv::size(points_); ++i)
        {
            points_[i] = space_.pos + space_.size / static_cast<float>(rowSize - 1) *
                                      glm::vec2(i % rowSize, i / rowSize);
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

        constexpr M2x2 shear{{1.f, 0.f}, {1.f, 1.f}};

        for(const auto point: points_)
        {
            hppv::Circle c;

            const auto tPoint = shear * point;
            assert(tPoint == glm::mat2(shear.i, shear.j) * point);

            c.center = glm::mix(point, tPoint, transition_.time / transition_.duration);
            c.center.y *= -1.f; // hack: in the Renderer coordinate system y grows down; we want the opposite
            c.radius = 0.065f;
            c.color = {1.f, 0.5f, 0.f, 1.f};
            renderer.cache(c);
        }

        ImGui::Begin("Matrix2x2");
        ImGui::Text("shear transformation");
        ImGui::NewLine();
        ImGui::Text("%.1f   %.1f\n\n"
                    "%.1f   %.1f", shear.i.x, shear.j.x, shear.i.y, shear.j.y);
        ImGui::End();
    }

private:
    static constexpr auto rowSize = 11;
    const hppv::Space space_{-5.f, -5.f, 10.f, 10.f};
    glm::vec2 points_[rowSize * rowSize];

    struct Transition
    {
        float time = 0.f;
        static inline auto duration = 10.f;
    }
    transition_;
};

RUN(Matrix2x2)
