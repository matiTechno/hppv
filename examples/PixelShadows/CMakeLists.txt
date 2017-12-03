add_executable(PixelShadows PixelShadows.cpp)
target_link_libraries(PixelShadows hppv)
file(COPY tile.png DESTINATION .)
