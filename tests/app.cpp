#include <hppv/App.hpp>

#include "catch.hpp"

TEST_CASE("App initialization")
{
    hppv::App app;
    REQUIRE(app.initialize(true));
}
