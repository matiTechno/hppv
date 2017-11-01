function(copyRes)
    file(COPY res DESTINATION .)
endfunction()

add_subdirectory(TestScene)
add_subdirectory(Shaders)
add_subdirectory(Lights)
