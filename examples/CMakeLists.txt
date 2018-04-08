set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

function(getRes)
    if(UNIX)
        # easier to work with shaders
        add_custom_target(symlink ALL COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_CURRENT_SOURCE_DIR}/res res)
    else()
        file(COPY res DESTINATION .)
    endif()
endfunction()

if(UNIX)
    set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)
endif()

add_subdirectory(TestScene)
add_subdirectory(Shaders)
add_subdirectory(Lights)
add_subdirectory(Particles)
add_subdirectory(PixelShadows)
add_subdirectory(GeometryLight)
add_subdirectory(GLTransitions)
add_subdirectory(Gravity)
add_subdirectory(OSM)
add_subdirectory(TrueType)
add_subdirectory(NormalMap)
add_subdirectory(Snake)
add_subdirectory(Imgui)
add_subdirectory(Fractal)
add_subdirectory(SoftBody)
add_subdirectory(Parallax)
add_subdirectory(DayNight)
add_subdirectory(Matrix2x2)

option(LUA_TEST "build LuaTest example" ON)

if(LUA_TEST)
    add_subdirectory(LuaTest)
endif()

add_subdirectory(RayTracer)

if(UNIX AND NOT APPLE)
    add_subdirectory(Audio)
endif()

option (SCENE3D "build Scene3D example" ON)

if(SCENE3D)
    add_subdirectory(Scene3D)
endif()
