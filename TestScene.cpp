#include "App.hpp"
#include "TestScene.hpp"

int main()
{
    App app;
    if(!app.initialize()) return 1;
    app.executeScene<TestScene>();
    return 0;
}
