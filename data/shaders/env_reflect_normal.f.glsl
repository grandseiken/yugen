uniform vec4 colour;
uniform ivec3 perlin_size;
uniform sampler3D perlin;
uniform float frame;
varying vec2 tex_coord;

#include "perlin.glsl"

const float normal_scaling = 0.1;
const float specular = 0.5;

void main()
{
  vec2 p = vec2(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  p = normal_scaling * (p - vec2(0.5, 0.5)) + vec2(0.5, 0.5);
  gl_FragColor = vec4(p.x, p.y, specular, 1.0);
}
