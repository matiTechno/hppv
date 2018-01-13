#include <hppv/Scene.hpp>
#include <hppv/Renderer.hpp>
#include <hppv/imgui.h>

#include "../run.hpp"

// inspired by 3Blue1Brown Essence of linear algebra

struct M2x2
{
    // j is inverted because y grows down in the Renderer coordinate system
    glm::vec2 i{1.f, 0.f}, j{0.f, -1.f};
};

glm::vec2 operator*(M2x2 m, glm::vec2 v)
{
    return m.i * v.x + m.j * v.y;
}

M2x2 mix(M2x2 x, M2x2 y, float a)
{
    x.i = (1.f - a) * x.i + a * y.i;
    x.j = (1.f - a) * x.j + a * y.j;
    return x;
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
            points_[i] = space_.pos + space_.size * glm::vec2(i % rowSize, i / rowSize) /
                                                    static_cast<float>(rowSize - 1);
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

        for(const auto point: points_)
        {
            hppv::Circle c;

            {
                constexpr M2x2 identity;
                M2x2 shear; shear.j.x = 1.f;
                shear = mix(identity, shear, transition_.time / transition_.duration);
                c.center = shear * point;
            }

            c.radius = 0.05f;
            c.color = {1.f, 0.5f, 0.f, 1.f};
            renderer.cache(c);
        }

        ImGui::Begin("Matrix2x2");
        ImGui::Text("shear transformation");
        ImGui::End();
    }

private:
    static constexpr auto rowSize = 10;
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
