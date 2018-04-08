#include <hppv/Renderer.hpp>

#include "sol.hpp"
#include "../run.hpp"

class LuaTest: public hppv::Scene
{
public:
    LuaTest()
    {
        properties_.maximize = true;

        lua_.open_libraries(sol::lib::math);
    }

    void render(hppv::Renderer& renderer) override
    {
        time_ += frame_.time;

        const hppv::Space space(0.f, 0.f, 100.f, 100.f);
        renderer.projection(hppv::expandToMatchAspectRatio(space, properties_.size));

        hppv::Sprite sprite;

        lua_["time"] = time_;
        lua_.script("x = 50 + math.sin(time) * 20"
                    "y = 50 + math.cos(time) * 20");

        sprite.pos = {lua_['x'], lua_['y']};
        sprite.size = {10.f, 10.f};
        sprite.color = {0.5f, 0.2f, 0.2f, 0.f};
        renderer.shader(hppv::Render::Color);
        renderer.cache(sprite);
    }

private:
    sol::state lua_;
    float time_ = 0.f;
};

RUN(LuaTest)
