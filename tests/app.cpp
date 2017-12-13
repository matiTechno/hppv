#include <hppv/App.hpp>

#include "catch.hpp"

TEST_CASE("App initialization")
{
    hppv::App app;
    hppv::App::InitParams initParams;
    initParams.printDebugInfo = true;
    REQUIRE(app.initialize(initParams));
}
