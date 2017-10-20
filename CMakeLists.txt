cmake_minimum_required(VERSION 3.0)
project(hppv)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -Wextra -pedantic -O2 -g -fno-rtti")
set(CMAKE_C_FLAGS   "-Wall -Wextra -O2")

add_definitions(-DGLM_FORCE_NO_CTOR_INIT)

include_directories(./include)

add_subdirectory(src)
add_subdirectory(examples)
enable_testing()
add_subdirectory(tests)
