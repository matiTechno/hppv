# sol2 clang warning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-null-dereference")

add_executable(LuaTest LuaTest.cpp sol.hpp)
target_link_libraries(LuaTest hppv lua)
