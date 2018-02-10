add_library(test_main test_main.cpp catch.hpp)
target_link_libraries(test_main PRIVATE hppv)

add_executable(test_app test_app.cpp)
target_link_libraries(test_app test_main)
add_test(NAME test_app COMMAND test_app)

add_executable(test_shader test_shader.cpp)
target_link_libraries(test_shader test_main)
add_test(NAME test_shader COMMAND test_shader)
file(COPY shaders DESTINATION .)
