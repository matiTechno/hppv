static const char* const fragStart = R"(

#fragment
#version 330

uniform float progress;
uniform float ratio;
uniform sampler2D samplerFrom;
uniform sampler2D samplerTo;

vec4 getFromColor(in vec2 uv)
{
    return texture(samplerFrom, uv);
}

vec4 getToColor(in vec2 uv)
{
    return texture(samplerTo, uv);
}
)";

static const char* const fragEnd = R"(

in vec2 vTexCoord;

out vec4 oColor;

void main()
{
    oColor = transition(vTexCoord);
    //oColor = getFromColor(vTexCoord);
}
)";

static const char* const burn = R"(

// author: gre
// License: MIT
uniform vec3 color = vec3(0.9, 0.4, 0.2);
vec4 transition (vec2 uv) {
  return mix(
    getFromColor(uv) + vec4(progress*color, 1.0),
    getToColor(uv) + vec4((1.0-progress)*color, 1.0),
    progress
    );
}
)";

static const char* const heart = R"(

// Author: gre
// License: MIT

float inHeart (vec2 p, vec2 center, float size) {
  if (size==0.0) return 0.0;
  vec2 o = (p-center)/(1.6*size);
  float a = o.x*o.x+o.y*o.y-0.3;
  return step(a*a*a, o.x*o.x*o.y*o.y*o.y);
}
vec4 transition (vec2 uv) {
  return mix(
    getFromColor(uv),
    getToColor(uv),
    inHeart(uv, vec2(0.5, 0.4), progress)
  );
}
)";

static const char* const cube = R"(

// Author: gre
// License: MIT
uniform float persp = 0.7;
uniform float unzoom = 0.3;
uniform float reflection = 0.4;
uniform float floating = 3.0;

vec2 project (vec2 p) {
  return p * vec2(1.0, -1.2) + vec2(0.0, -floating/100.);
}

bool inBounds (vec2 p) {
  return all(lessThan(vec2(0.0), p)) && all(lessThan(p, vec2(1.0)));
}

vec4 bgColor (vec2 p, vec2 pfr, vec2 pto) {
  vec4 c = vec4(0.0, 0.0, 0.0, 1.0);
  pfr = project(pfr);
  // FIXME avoid branching might help perf!
  if (inBounds(pfr)) {
    c += mix(vec4(0.0), getFromColor(pfr), reflection * mix(1.0, 0.0, pfr.y));
  }
  pto = project(pto);
  if (inBounds(pto)) {
    c += mix(vec4(0.0), getToColor(pto), reflection * mix(1.0, 0.0, pto.y));
  }
  return c;
}

// p : the position
// persp : the perspective in [ 0, 1 ]
// center : the xcenter in [0, 1] \ 0.5 excluded
vec2 xskew (vec2 p, float persp, float center) {
  float x = mix(p.x, 1.0-p.x, center);
  return (
    (
      vec2( x, (p.y - 0.5*(1.0-persp) * x) / (1.0+(persp-1.0)*x) )
      - vec2(0.5-distance(center, 0.5), 0.0)
    )
    * vec2(0.5 / distance(center, 0.5) * (center<0.5 ? 1.0 : -1.0), 1.0)
    + vec2(center<0.5 ? 0.0 : 1.0, 0.0)
  );
}

vec4 transition(vec2 op) {
  float uz = unzoom * 2.0*(0.5-distance(0.5, progress));
  vec2 p = -uz*0.5+(1.0+uz) * op;
  vec2 fromP = xskew(
    (p - vec2(progress, 0.0)) / vec2(1.0-progress, 1.0),
    1.0-mix(progress, 0.0, persp),
    0.0
  );
  vec2 toP = xskew(
    p / vec2(progress, 1.0),
    mix(pow(progress, 2.0), 1.0, persp),
    1.0
  );
  // FIXME avoid branching might help perf!
  if (inBounds(fromP)) {
    return getFromColor(fromP);
  }
  else if (inBounds(toP)) {
    return getToColor(toP);
  }
  return bgColor(op, fromP, toP);
}
)";

static const char* const pixelize = R"(

// Author: gre
// License: MIT
// forked from https://gist.github.com/benraziel/c528607361d90a072e98

uniform ivec2 squaresMin = ivec2(20); // minimum number of squares (when the effect is at its higher level)
uniform int steps = 50; // zero disable the stepping

float d = min(progress, 1.0 - progress);
float dist = steps>0 ? ceil(d * float(steps)) / float(steps) : d;
vec2 squareSize = 2.0 * dist / vec2(squaresMin);

vec4 transition(vec2 uv) {
  vec2 p = dist>0.0 ? (floor(uv / squareSize) + 0.5) * squareSize : uv;
  return mix(getFromColor(p), getToColor(p), progress);
}
)";

static const char* const ripple = R"(

// Author: gre
// License: MIT
uniform float amplitude; // = 100.0
uniform float speed; // = 50.0

vec4 transition (vec2 uv) {
  vec2 dir = uv - vec2(.5);
  float dist = length(dir);
  vec2 offset = dir * (sin(progress * dist * amplitude - progress * speed) + .5) / 30.;
  return mix(
    getFromColor(uv + offset),
    getToColor(uv),
    smoothstep(0.2, 1.0, progress)
  );
}
)";

static const char* const hexagonalize = R"(

// Author: Fernando Kuteken
// License: MIT
// Hexagonal math from: http://www.redblobgames.com/grids/hexagons/

uniform int steps = 50;
uniform float horizontalHexagons = 20;

struct Hexagon {
  float q;
  float r;
  float s;
};

Hexagon createHexagon(float q, float r){
  Hexagon hex;
  hex.q = q;
  hex.r = r;
  hex.s = -q - r;
  return hex;
}

Hexagon roundHexagon(Hexagon hex){

  float q = floor(hex.q + 0.5);
  float r = floor(hex.r + 0.5);
  float s = floor(hex.s + 0.5);

  float deltaQ = abs(q - hex.q);
  float deltaR = abs(r - hex.r);
  float deltaS = abs(s - hex.s);

  if (deltaQ > deltaR && deltaQ > deltaS)
    q = -r - s;
  else if (deltaR > deltaS)
    r = -q - s;
  else
    s = -q - r;

  return createHexagon(q, r);
}

Hexagon hexagonFromPoint(vec2 point, float size) {

  point.y /= ratio;
  point = (point - 0.5) / size;

  float q = (sqrt(3.0) / 3.0) * point.x + (-1.0 / 3.0) * point.y;
  float r = 0.0 * point.x + 2.0 / 3.0 * point.y;

  Hexagon hex = createHexagon(q, r);
  return roundHexagon(hex);

}

vec2 pointFromHexagon(Hexagon hex, float size) {

  float x = (sqrt(3.0) * hex.q + (sqrt(3.0) / 2.0) * hex.r) * size + 0.5;
  float y = (0.0 * hex.q + (3.0 / 2.0) * hex.r) * size + 0.5;

  return vec2(x, y * ratio);
}

vec4 transition (vec2 uv) {

  float dist = 2.0 * min(progress, 1.0 - progress);
  dist = steps > 0 ? ceil(dist * float(steps)) / float(steps) : dist;

  float size = (sqrt(3.0) / 3.0) * dist / horizontalHexagons;

  vec2 point = dist > 0.0 ? pointFromHexagon(hexagonFromPoint(uv, size), size) : uv;

  return mix(getFromColor(point), getToColor(point), progress);

}
)";
