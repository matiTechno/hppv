add_executable(DayNight DayNight.cpp)
target_link_libraries(DayNight hppv)
file(COPY data.txt DESTINATION .)
