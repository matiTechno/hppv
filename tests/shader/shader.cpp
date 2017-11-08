#include <string>

#include <hppv/App.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>

#include "../catch.hpp"

static const char* vertex = R"(

#vertex

#version 330

layout(location = 0) in vec2 pos;

void main()
{
    gl_Position = vec4(pos, 0, 1);
}
)";

static const char* fragment = R"(

#fragment

#version 330

out vec4 color;

void main()
{
    color = vec4(1, 0, 0, 1);
}
)";

TEST_CASE("shader")
{
    hppv::App app;
    REQUIRE(app.initialize(false));

    hppv::Shader shader(hppv::File(), "shaders/fragment.sh");
    REQUIRE(shader.isValid());

    shader = hppv::Shader(hppv::File(), "shaders/vertex.sh");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({vertex, fragment}, "1");
    REQUIRE(shader.isValid());

    shader = hppv::Shader(hppv::File(), "fragment.sh", true);
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({vertex, fragment, fragment}, "2");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({fragment, std::string(vertex)}, "3");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({}, "4");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({hppv::Renderer::vertexSource, fragment, std::string()}, "5");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({fragment + std::string("error")}, "6");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({vertex, fragment, hppv::Renderer::vertexSource}, "7");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({fragment}, "8");
    REQUIRE(shader.isValid());

    hppv::Shader shader2(std::move(shader));
    REQUIRE(shader2.isValid());
    REQUIRE(shader.isValid() == false);
}
