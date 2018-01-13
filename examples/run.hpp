#include <hppv/App.hpp>

#define RUN(SceneType) \
    int main() { \
    hppv::App app; \
    hppv::App::InitParams p; \
    p.window.title = #SceneType; \
    if(!app.initialize(p)) return 1; \
    app.pushScene<SceneType>(); \
    app.run(); \
    return 0;}
