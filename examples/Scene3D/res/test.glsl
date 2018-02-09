#vertex
#version 330

layout(location = 0) in vec3 vertex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(vertex, 1.0);
}

#fragment
#version 330

out vec4 color;

void main()
{
    color = vec4(1.0, 1.0, 0.0, 1.0);
}
