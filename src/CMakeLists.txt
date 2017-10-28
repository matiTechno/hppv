set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

add_library(hppv

    App.cpp
    Font.cpp
    Framebuffer.cpp
    GLobjects.cpp
    PrototypeScene.cpp
    Renderer.cpp
    Scene.cpp
    Space.cpp
    Texture.cpp

    external/glad.c

    external/stb_image.h

    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_impl_glfw_gl3.cpp
    external/imgui/imgui_impl_glfw_gl3.h
    external/imgui/imgui_internal.h
    external/imgui/stb_rect_pack.h
    external/imgui/stb_textedit.h
    external/imgui/stb_truetype.h
    )

target_include_directories(hppv PRIVATE ../include/hppv/external)
target_link_libraries(hppv PUBLIC -lglfw PRIVATE -lGL -ldl -lstdc++fs)
