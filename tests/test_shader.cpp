#include <string>

#include <hppv/App.hpp>
#include <hppv/Shader.hpp>
#include <hppv/Renderer.hpp>

#include "catch.hpp"

const char* const vertex = R"(

#vertex

#version 330

layout(location = 0) in vec2 pos;

void main()
{
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* const fragment = R"(

#fragment

#version 330

out vec4 color;

void main()
{
    color = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

TEST_CASE("shader")
{
    hppv::App app;
    REQUIRE(app.initialize({}));

    hppv::Shader shader(hppv::Shader::File(), "shaders/fragment.sh");
    REQUIRE(shader.isValid());
    REQUIRE(shader.getId() == "shaders/fragment.sh");

    shader = hppv::Shader(hppv::Shader::File(), "shaders/vertex.sh");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({vertex, fragment}, "1");
    REQUIRE(shader.isValid());
    REQUIRE(shader.getId() == "1");

    shader = hppv::Shader(hppv::Shader::File(), "fragment.sh", true);
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({vertex, fragment, fragment}, "2");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({fragment, std::string(vertex)}, "3");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({}, "4");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({hppv::Renderer::vInstancesSource, fragment, std::string()}, "5");
    REQUIRE(shader.isValid());

    shader = hppv::Shader({fragment + std::string("error")}, "6");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({vertex, fragment, hppv::Renderer::vInstancesSource}, "7");
    REQUIRE(shader.isValid() == false);

    shader = hppv::Shader({fragment}, "8");
    REQUIRE(shader.isValid());

    hppv::Shader shader2(std::move(shader));
    REQUIRE(shader2.isValid());
    REQUIRE(shader2.getId() == "8");
    REQUIRE(shader.isValid() == false);

    shader2 = hppv::Shader({}, "9");
    REQUIRE(shader.isValid() == false);

    shader2 = hppv::Shader();
    REQUIRE(shader.isValid() == false);
}
