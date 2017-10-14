cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -O2 -g")
set(CMAKE_C_FLAGS "-Wall -Wextra -O2 -g")

add_executable(testscene
    TestScene.cpp
    TestScene.hpp

    App.cpp
    App.hpp
    Deleter.hpp
    glad.c
    glad.h
    Renderer.cpp
    Renderer.hpp
    Scene.hpp
    Scene.cpp
    Shader.hpp
    Space.cpp
    Space.hpp
    )

target_link_libraries(testscene PRIVATE -lglfw -lGL -ldl -lstdc++fs)
target_compile_definitions(testscene PRIVATE GLM_FORCE_NO_CTOR_INIT)
