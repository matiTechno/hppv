#include <hppv/external/catch.hpp>

#include <hppv/App.hpp>

TEST_CASE("App initialization")
{
    hppv::App app;
    REQUIRE(app.initialize(true));
}
