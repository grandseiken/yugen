uniform vec4 colour;
uniform ivec3 perlin_size;
uniform ivec2 resolution;
uniform sampler3D perlin;
uniform sampler2D source;
uniform float frame;

varying vec2 tex_coord;
varying vec2 reflect_coord;
varying vec2 refract_coord;

#include "perlin.glsl"

const float normal_scaling = 2.0;

void main()
{
  // Calculate texture offset based on perlin noise.
  vec2 p = vec2(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  p = 2.0 * (p - vec2(0.5, 0.5));
  p *= normal_scaling / vec2(resolution);

  // Mix reflected, refracted and colour.
  // TODO: this is very naive, I wonder if more correct reflection would look
  // nicer.
  // TODO: water needs specular to look good.
  vec4 reflect = texture2D(source, reflect_coord + 2.0 * p);
  vec4 refract = texture2D(source, refract_coord - p);

  // TODO: reduce reflection the further we get from the "surface".
  // TODO: allow custom mixing values (for mirrors, etc).
  float a = colour.a;
  vec4 c = a * (reflect + colour) / 2.0 + (1 - a) * refract;
  gl_FragColor = vec4(c.r, c.g, c.b, 1.0);
}
