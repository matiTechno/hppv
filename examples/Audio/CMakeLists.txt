add_executable(Audio Audio.cpp)
target_link_libraries(Audio hppv ${CMAKE_CURRENT_SOURCE_DIR}/fmod/libfmod.so)
getRes()
