set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
add_library(hppv
    App.cpp
    glad.c
    GLobjects.cpp
    PrototypeScene.cpp
    Renderer.cpp
    Scene.cpp
    Space.cpp
    Texture.cpp

    stb_image.h

    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_glfw_gl3.cpp
    imgui/imgui_impl_glfw_gl3.h
    imgui/imgui_internal.h
    imgui/stb_rect_pack.h
    imgui/stb_textedit.h
    imgui/stb_truetype.h
    )

target_include_directories(hppv PRIVATE ../include/hppv)
target_link_libraries(hppv PUBLIC -lglfw PRIVATE -lGL -ldl -lstdc++fs)
