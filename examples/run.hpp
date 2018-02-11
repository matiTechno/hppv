#include <hppv/App.hpp>

#define RUN(SceneType) \
    int main() { \
    hppv::App app; \
    hppv::App::InitParams p; \
    p.window.title = #SceneType; \
    p.window.state = hppv::Window::Fullscreen; \
    if(!app.initialize(p)) return 1; \
    app.pushScene<SceneType>(); \
    app.run(); \
    return 0;}
