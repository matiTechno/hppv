add_library(tests-main tests-main.cpp catch.hpp)
target_link_libraries(tests-main PRIVATE hppv)

add_executable(app app.cpp)
target_link_libraries(app tests-main)

add_test(NAME app COMMAND app)
