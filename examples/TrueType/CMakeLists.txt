add_executable(TrueType TrueType.cpp)
target_link_libraries(TrueType hppv)
file(COPY SourceCodePro-Regular.otf ProggyClean.ttf DESTINATION .)
