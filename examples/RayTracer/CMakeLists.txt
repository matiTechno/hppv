set(CMAKE_CXX_FLAGS "-g -std=c++17")
add_executable(RayTracer RayTracer.cpp renderImage.cpp renderImage.hpp)
target_link_libraries(RayTracer hppv pthread)
copyRes()
