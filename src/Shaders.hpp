#include "Renderer.hpp"

const char* hppv::Renderer::vertexSource = R"(

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
out vec2 vTexCoords;
out vec2 vPos;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vPos = vertex.xy;

    vec2 texCoords = vertex.zw;

    if(flipTexRectX)
    {
        texCoords.x = (texCoords.x - 1) * -1;
    }

    if(flipTexRectY)
    {
        texCoords.y = (texCoords.y - 1) * -1;
    }

    vTexCoords = texCoords * normTexRect.zw + normTexRect.xy;

    if(flipTextureY)
    {
        vTexCoords.y = 1 - vTexCoords.y;
    }
}
)";

static const char* basicSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;
in vec2 vPos;

uniform sampler2D sampler;
uniform int mode = 0;
uniform bool premultiplyAlpha = false;
uniform bool antialiasedSprites = false;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void rectAlpha(out float alpha)
{
    float deltaX = fwidth(length(vPos.x - center.x)) * 2;
    float distX = abs(vPos.x - center.x);
    alpha = 1 - smoothstep(0.5 - deltaX, 0.5, distX);

    float deltaY = fwidth(length(vPos.y - center.y)) * 2;
    float distY = abs(vPos.y - center.y);
    alpha *= 1 - smoothstep(0.5 - deltaY, 0.5, distY);
}

void circleAlpha(out float alpha)
{
    float distanceFromCenter = length(vPos - center);
    float delta = fwidth(distanceFromCenter) * 2;
    alpha = 1 - smoothstep(radius - delta, radius, distanceFromCenter);
}

void main()
{
    if(mode == 0) // Color
    {
        float alpha = 1;

        if(antialiasedSprites)
        {
            rectAlpha(alpha);
        }

        color = vColor * alpha;
    }
    else if(mode == 2) // CircleColor
    {
        float alpha;
        circleAlpha(alpha);
        color = vColor * alpha;
    }
    else
    {
        vec4 sample = texture(sampler, vTexCoords);

        if(premultiplyAlpha)
        {
            sample = vec4(sample.rgb * sample.a, sample.a);
        }

        if(mode == 1) // Tex
        {
            float alpha = 1;

            if(antialiasedSprites)
            {
                rectAlpha(alpha);
            }

            color = sample * vColor * alpha;
        }
        else if(mode == 3) // CircleTex
        {
            float alpha;
            circleAlpha(alpha);
            color = sample * vColor * alpha;
        }
    }
}
)";

static const char* sdfSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;
in vec2 vPos;

uniform sampler2D sampler;
uniform int mode;
uniform vec4 outlineColor = vec4(1, 0, 0, 1);
uniform float outlineWidth = 0.25; // [0, 0.5]
uniform vec4 glowColor = vec4(1, 0, 0, 1);
uniform float glowWidth = 0.5; // [0, 0.5]
uniform vec4 shadowColor = vec4(1, 0, 0, 1);
uniform float shadowSmoothing = 0.2; // [0, 0.5]
uniform vec2 shadowOffset = vec2(-0.003, -0.006);

const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float smoothing = fwidth(length(vPos - center)) * 2;
    float distance = texture(sampler, vTexCoords).a;

    if(mode == 4) // Sdf
    {
        float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        color = vColor * alpha;
    }
    else if(mode == 5) // SdfOutline
    {
        float outlineFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        float oOutlineWidth = 0.5 - outlineWidth;
        float alpha = smoothstep(oOutlineWidth - smoothing, oOutlineWidth + smoothing, distance);
        color = mix(outlineColor, vColor, outlineFactor) * alpha;
    }
    else if(mode == 6) // SdfGlow
    {
        float glowFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        float glowSmoothing = max(smoothing, glowWidth);
        float alpha = smoothstep(0.5 - glowSmoothing, 0.5 + smoothing, distance);
        color = mix(glowColor, vColor, glowFactor) * alpha;
    }
    else if(mode == 7) // SdfShadow
    {
        float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
        vec4 textColor = vColor * alpha;

        float shadowDistance = texture(sampler, vTexCoords - shadowOffset).a;
        float oShadowSmoothing = max(smoothing, shadowSmoothing);
        float shadowAlpha = smoothstep(0.5 - oShadowSmoothing, 0.5 + oShadowSmoothing, shadowDistance);
        vec4 shadow = shadowColor * shadowAlpha;

        color = mix(shadow, textColor, textColor.a);
    }
}
)";
