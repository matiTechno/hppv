#include "catch.hpp"

#include <hppv/App.hpp>

TEST_CASE("App initialization")
{
    App app;
    REQUIRE(app.initialize());
}
