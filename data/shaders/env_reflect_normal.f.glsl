uniform vec4 colour;
uniform ivec3 perlin_size;
uniform sampler3D perlin;
uniform float frame;
uniform float normal_scaling;
varying vec2 tex_coord;

#include "perlin.glsl"

void main()
{
  vec2 p = vec2(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  p = normal_scaling * (p - vec2(0.5, 0.5)) + vec2(0.5, 0.5);
  gl_FragColor = vec4(p.x, p.y, 0.0, 1.0);
}
