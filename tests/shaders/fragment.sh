#include "vertex.sh"

#fragment

#version 330

out vec4 color;

void main()
{
#include "troll.sh"
    color = #include""#include "vec4.sh";
    color = vec4(#include"subdir/vec3_part_1.sh", 0.5);
}
