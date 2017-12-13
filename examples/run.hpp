#include <hppv/App.hpp>

#define RUN(SceneType) \
    int main() { \
    hppv::App app; \
    if(!app.initialize({})) return 1; \
    app.pushScene<SceneType>(); \
    app.run(); \
    return 0;}
