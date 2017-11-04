add_executable(shader shader.cpp)
target_link_libraries(shader tests-main)
file(COPY shaders DESTINATION .)
add_test(NAME shader COMMAND shader)
