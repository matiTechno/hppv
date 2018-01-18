const char* const deferredSource = R"(

#fragment
#version 330

uniform sampler2D samplerDiffuse;
uniform sampler2D samplerNormal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 attCoeffs;
uniform vec2 projPos;
uniform vec2 projSize;

in vec2 vTexCoord;
in vec2 vPos;

out vec4 color;

void main()
{
    vec4 sampleDiffuse = texture(samplerDiffuse, vTexCoord);
    vec3 sampleNormal = texture(samplerNormal, vTexCoord).rgb;

    vec3 fragPos = vec3(vPos * projSize + projPos, 0);

    vec3 lightDir = lightPos - fragPos;

    float D = length(lightDir);
    float attenuation = 1 / (attCoeffs.x + attCoeffs.y * D + attCoeffs.z * D * D);

    vec3 N = normalize(sampleNormal * 2 - 1);
    vec3 L = normalize(lightDir);
    L.y = -L.y;

    color = vec4(sampleDiffuse.rgb * (lightColor * max(dot(L, N), 0) * attenuation + ambientColor),
                 sampleDiffuse.a);
}
)";

const char* const textureSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D samplerDiffuse;
uniform sampler2D samplerNormal;

uniform bool invertNormalY = false;

layout(location = 0) out vec4 colorDiffuse;
layout(location = 1) out vec4 colorNormal;

void main()
{
    vec4 sampleDiffuse = texture(samplerDiffuse, vTexCoord);

    // premultiplied alpha
    sampleDiffuse = vec4(sampleDiffuse.rgb * sampleDiffuse.a, sampleDiffuse.a);

    colorDiffuse = sampleDiffuse * vColor;

    colorNormal = texture(samplerNormal, vTexCoord);

    if(invertNormalY)
    {
        colorNormal.g = 1 - colorNormal.g;
    }
}
)";
