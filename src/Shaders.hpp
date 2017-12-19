#include "Renderer.hpp"

const char* const hppv::Renderer::vInstancesSource = R"(

#vertex
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 normTexRect;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;
uniform bool flipTexRectX = false;
uniform bool flipTexRectY = false;
uniform bool flipTextureY = false;

out vec4 vColor;
out vec2 vTexCoord;
out vec2 vPos;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vPos = vertex.xy;

    vec2 texCoord = vertex.zw;

    if(flipTexRectX)
    {
        texCoord.x = (texCoord.x - 1) * -1;
    }

    if(flipTexRectY)
    {
        texCoord.y = (texCoord.y - 1) * -1;
    }

    vTexCoord = texCoord * normTexRect.zw + normTexRect.xy;

    if(flipTextureY)
    {
        vTexCoord.y = 1 - vTexCoord.y;
    }
}
)";

const char* const fBasicSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;
in vec2 vPos;

uniform sampler2D sampler;
uniform int mode = 0;
uniform bool premultiplyAlpha = false;
uniform bool antialiasedSprites = false;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

float rectAlpha()
{
    float deltaX = fwidth(length(vPos.x - center.x)) * 2;
    float distX = abs(vPos.x - center.x);
    float alpha = 1 - smoothstep(0.5 - deltaX, 0.5, distX);
    float deltaY = fwidth(length(vPos.y - center.y)) * 2;
    float distY = abs(vPos.y - center.y);
    alpha *= 1 - smoothstep(0.5 - deltaY, 0.5, distY);
    return alpha;
}

float circleAlpha()
{
    float distanceFromCenter = length(vPos - center);
    float delta = fwidth(distanceFromCenter) * 2;
    return 1 - smoothstep(radius - delta, radius, distanceFromCenter);
}

void main()
{
    if(mode == 0) // Color
    {
        float alpha = 1;

        if(antialiasedSprites)
        {
            alpha = rectAlpha();
        }

        color = vColor * alpha;
    }
    else if(mode == 2) // CircleColor
    {
        color = vColor * circleAlpha();
    }
    else
    {
        vec4 sample = texture(sampler, vTexCoord);

        if(mode == 4) // Font
        {
            color = vec4(sample.r) * vColor;
            return;
        }

        if(premultiplyAlpha)
        {
            sample = vec4(sample.rgb * sample.a, sample.a);
        }

        if(mode == 1) // Tex
        {
            float alpha = 1;

            if(antialiasedSprites)
            {
                alpha = rectAlpha();
            }

            color = sample * vColor * alpha;
        }
        else if(mode == 3) // CircleTex
        {
            color = sample * vColor * circleAlpha();
        }
    }
}
)";

const char* const fSdfSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;
in vec2 vPos;

uniform sampler2D sampler;
uniform int mode;
uniform vec4 outlineColor = vec4(1, 0, 0, 1);
uniform float outlineWidth = 0.25; // [0, 0.5]
uniform vec4 glowColor = vec4(1, 0, 0, 1);
uniform float glowWidth = 0.5; // [0, 0.5]
uniform vec4 shadowColor = vec4(1, 0, 0, 1);
uniform float shadowSmoothing = 0.2; // [0, 0.5]
uniform vec2 shadowOffset = vec2(-0.003, 0.006);

const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float smoothing = fwidth(length(vPos - center)) * 2;
    float distance = texture(sampler, vTexCoord).a;

    if(mode == 5) // Sdf
    {
        float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        color = vColor * alpha;
    }
    else if(mode == 6) // SdfOutline
    {
        float outlineFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        float oOutlineWidth = 0.5 - outlineWidth;
        float alpha = smoothstep(oOutlineWidth - smoothing, oOutlineWidth + smoothing, distance);
        color = mix(outlineColor, vColor, outlineFactor) * alpha;
    }
    else if(mode == 7) // SdfGlow
    {
        float glowFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        float glowSmoothing = max(smoothing, glowWidth);
        float alpha = smoothstep(0.5 - glowSmoothing, 0.5 + smoothing, distance);
        color = mix(glowColor, vColor, glowFactor) * alpha;
    }
    else if(mode == 8) // SdfShadow
    {
        float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        vec4 textColor = vColor * alpha;

        float shadowDistance = texture(sampler, vTexCoord - vec2(shadowOffset.x, -shadowOffset.y)).a;
        float oShadowSmoothing = max(smoothing, shadowSmoothing);
        float shadowAlpha = smoothstep(0.5 - oShadowSmoothing, 0.5 + oShadowSmoothing, shadowDistance);
        vec4 shadow = shadowColor * shadowAlpha;

        color = mix(shadow, textColor, textColor.a);
    }
}
)";

const char* const hppv::Renderer::vVerticesSource = R"(

#vertex
#version 330

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec4 color;

uniform mat4 projection;
uniform bool flipTextureY = false;

out vec4 vColor;
out vec2 vTexCoord;

void main()
{
    gl_Position = projection * vec4(pos, 0, 1);
    vTexCoord = texCoord;
    vColor = color;

    if(flipTextureY)
    {
        vTexCoord.y = 1 - vTexCoord.y;
    }
}
)";

const char* const fVerticesSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sampler;
uniform int mode = 8;
uniform bool premultiplyAlpha = false;

out vec4 color;

void main()
{
    if(mode == 9) // VerticesColor
    {
        color = vColor;
    }
    else if(mode == 10) // VerticesTex
    {
        vec4 sample = texture(sampler, vTexCoord);

        if(premultiplyAlpha)
        {
            sample = vec4(sample.rgb * sample.a, sample.a);
        }

        color = sample * vColor;
    }
}
)";
