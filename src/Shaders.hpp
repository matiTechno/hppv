#include "Renderer.hpp"

const char* hppv::Renderer::vertexSource = R"(

#vertex
#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;
layout(location = 2) in vec4 normTexRect;
layout(location = 3) in mat4 matrix;

uniform mat4 projection;

out vec4 vColor;
out vec2 vTexCoords;
out vec2 vPosition;

void main()
{
    gl_Position = projection * matrix * vec4(vertex.xy, 0, 1);
    vColor = color;
    vTexCoords = vertex.zw * normTexRect.zw + normTexRect.xy;
    vPosition = vertex.xy;
}
)";

static const char* colorSource = R"(

#fragment
#version 330

in vec4 vColor;

out vec4 color;

void main()
{
    color = vColor;
}
)";

static const char* texSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;

out vec4 color;

void main()
{
    color = texture(sampler, vTexCoords) * vColor;
}
)";

static const char* texPremultiplyAlphaSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vTexCoords;

uniform sampler2D sampler;

out vec4 color;

void main()
{
    vec4 sample = texture(sampler, vTexCoords);
    color = vec4(sample.rgb * sample.a, sample.a) * vColor;
}
)";

static const char* circleColorSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = 1 - smoothstep(radius - delta * 2,
                                 radius,
                                 distanceFromCenter);

    color = vColor * alpha;
}
)";

static const char* circleTexSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = 1 - smoothstep(radius - delta * 2,
                                 radius,
                                 distanceFromCenter);

    color = texture(sampler, vTexCoords) * vColor * alpha;
}
)";

static const char* circleTexPremultiplyAlphaSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;

const float radius = 0.5;
const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float distanceFromCenter = length(vPosition - center);
    float delta = fwidth(distanceFromCenter);
    float alpha = 1 - smoothstep(radius - delta * 2,
                                 radius,
                                 distanceFromCenter);

    vec4 sample = texture(sampler, vTexCoords);
    color = vec4(sample.rgb * sample.a, sample.a) * vColor * alpha;
}
)";

static const char* fontSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;

const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float smoothing = fwidth(length(vPosition - center) * 2);
    float distance = texture(sampler, vTexCoords).a;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    color = vColor * alpha;
}
)";

static const char* fontOutlineSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;
uniform float outlineWidth = 0.2; // 0 - 0.5
uniform vec4 outlineColor = vec4(1, 0, 0, 1);

const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float smoothing = fwidth(length(vPosition - center) * 2);
    float distance = texture(sampler, vTexCoords).a;
    float outlineFactor = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    float alpha = smoothstep(outlineWidth - smoothing, outlineWidth + smoothing, distance);
    color = mix(outlineColor, vColor, outlineFactor) * alpha;
}
)";

static const char* fontShadowSource = R"(

#fragment
#version 330

in vec4 vColor;
in vec2 vPosition;
in vec2 vTexCoords;

uniform sampler2D sampler;
uniform vec2 shadowOffset = vec2(-0.002, -0.002);
uniform float shadowSmoothing = 0.5; // 0 - 0.5
uniform vec4 shadowColor = vec4(1, 0, 0, 1);

const vec2 center = vec2(0.5, 0.5);

out vec4 color;

void main()
{
    float smoothing = fwidth(length(vPosition - center) * 2);
    float distance = texture(sampler, vTexCoords).a;
    float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, distance);
    vec4 textColor = vColor * alpha;

    float shadowDistance = texture(sampler, vTexCoords - shadowOffset).a;
    float shadowAlpha = smoothstep(0.5 - shadowSmoothing, 0.5 + shadowSmoothing, shadowDistance);
    vec4 shadow = shadowColor * shadowAlpha;

    gl_FragColor = mix(shadow, textColor, textColor.a);
}
)";
