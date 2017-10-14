cmake_minimum_required(VERSION 3.0)
project(hppv)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -O2 -g -pipe")
set(CMAKE_C_FLAGS "-Wall -Wextra -O2 -g -pipe")

add_executable(testscene TestScene.cpp TestScene.hpp)
target_link_libraries(testscene hppv)

add_library(hppv
    App.cpp
    App.hpp
    Deleter.hpp
    Event.hpp
    glad.c
    glad.h
    PrototypeScene.cpp
    PrototypeScene.hpp
    Renderer.cpp
    Renderer.hpp
    Scene.hpp
    Scene.cpp
    Shader.hpp
    Space.cpp
    Space.hpp

    imgui/imconfig.h
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui.h
    imgui/imgui_impl_glfw_gl3.cpp
    imgui/imgui_impl_glfw_gl3.h
    imgui/imgui_internal.h
    imgui/stb_rect_pack.h
    imgui/stb_textedit.h
    imgui/stb_truetype.h
    )

target_link_libraries(hppv PUBLIC -lglfw PRIVATE -lGL -ldl -lstdc++fs)
target_compile_definitions(hppv PUBLIC GLM_FORCE_NO_CTOR_INIT)
