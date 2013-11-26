uniform vec4 colour;
uniform ivec3 perlin_size;
uniform sampler3D perlin;
uniform float frame;
uniform float fog_min = 0.4;
uniform float fog_max = 0.7;
noperspective varying vec2 tex_coord;

#include "../perlin.glsl"

const bool flatten_fog = false;
float fog_scale = 1.0 / (fog_max - fog_min);
float flatten_val = (fog_min + fog_max) / 2.0;

void main()
{
  float p = float(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  if (p < fog_min || (flatten_fog && p < flatten_val)) {
    discard;
  }
  vec4 c = colour;
  c.a *= flatten_fog ? 1.0 : fog_scale * (p - fog_min);
  gl_FragColor = c;
}
