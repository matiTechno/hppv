add_executable(Snake
    main.cpp
    Background.cpp
    Background.hpp
    Menu.cpp
    Menu.hpp
    Snake.cpp
    Snake.hpp)

target_link_libraries(Snake hppv)
file(COPY food.png "CPMono_v07 Bold.otf" DESTINATION .)
