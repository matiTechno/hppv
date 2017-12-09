cmake_minimum_required(VERSION 3.0)
project(hppv)

option(EXAMPLES "build examples" ON)
option(TESTS "build tests" ON)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -O2 -g")
set(CMAKE_C_FLAGS   "-Wall -Wextra -O2")

add_definitions(-DGLM_FORCE_NO_CTOR_INIT -DSHADER_GLM)

include_directories(./include)

add_subdirectory(src)

if(EXAMPLES)
    add_subdirectory(examples)
endif()

if(TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
