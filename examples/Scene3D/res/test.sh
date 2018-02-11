#vertex
#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 vPos;
out vec2 vTexCoord;
out vec3 vNormal;

void main()
{
    vec4 pos = model * vec4(vertex, 1.0);
    vPos = pos.xyz;
    gl_Position = projection * view * pos;
    vTexCoord = texCoord;
    vNormal = mat3(transpose(inverse(model))) * normal;
}

#fragment
#version 330

uniform mat4 view;
uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
uniform bool useTexture = false;
uniform sampler2D diffuse;
uniform bool doLighting = false;
uniform vec3 lightPos;
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);

in vec3 vPos;
in vec2 vTexCoord;
in vec3 vNormal;

out vec4 oColor;

void main()
{
    oColor = color;

    if(useTexture)
    {
        oColor *= texture2D(diffuse, vTexCoord);
    }

    if(doLighting)
    {
        vec3 lightDir = normalize(vPos - lightPos);
        // isn't vNormal already normalized?
        oColor.xyz *= lightColor * dot(-lightDir, normalize(vNormal));
    }
}
