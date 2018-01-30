cmake_minimum_required(VERSION 3.0)
project(hppv)

option(EXAMPLES "build examples" ON)
option(TESTS "build tests" ON)

# hack? I want to keep the asserts
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2")

# -----
# blog.kitware.com/cmake-and-the-default-build-type

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# -----

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS   "-Wall -Wextra -O3")

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
