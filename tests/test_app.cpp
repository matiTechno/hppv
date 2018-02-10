#include <hppv/App.hpp>

#include "catch.hpp"

TEST_CASE("App initialization")
{
    hppv::App app;
    hppv::App::InitParams p;
    p.printDebugInfo = true;
    REQUIRE(app.initialize(p));
}
