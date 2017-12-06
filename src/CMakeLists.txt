set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

add_library(hppv

    App.cpp
    Font.cpp
    Framebuffer.cpp
    GLobjects.cpp
    PrototypeScene.cpp
    Renderer.cpp
    Scene.cpp
    Shaders.hpp
    Space.cpp
    Texture.cpp

    glad.c

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

target_include_directories(hppv PRIVATE ../include/hppv) # for imgui

target_link_libraries(hppv PRIVATE -lglfw -lGL -ldl -lstdc++fs)

target_compile_definitions(hppv PRIVATE
    IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
    IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION)
