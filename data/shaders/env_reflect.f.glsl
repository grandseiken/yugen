uniform vec4 colour;
uniform ivec3 perlin_size;
uniform ivec2 resolution;
uniform sampler3D perlin;
uniform sampler2D source;
uniform float frame;
uniform float reflect_mix;
uniform float normal_scaling_reflect;
uniform float normal_scaling_refract;
uniform float reflect_fade_start;
uniform float reflect_fade_end;

varying vec2 tex_coord;
varying vec2 reflect_coord;
varying vec2 refract_coord;
varying float reflect_dist;

#include "perlin.glsl"

void main()
{
  // Calculate texture offset based on perlin noise.
  vec2 p = vec2(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  p = 2.0 * (p - vec2(0.5, 0.5));
  p /= vec2(resolution);

  // Mix reflected, refracted and colour.
  // TODO: this is very naive, I wonder if more correct reflection would look
  // nicer.
  // TODO: water needs specular to look good?
  vec4 reflect = texture2D(
      source, reflect_coord + normal_scaling_reflect * p);
  vec4 refract = texture2D(
      source, refract_coord - normal_scaling_refract * p);

  // Calculate reflection coefficient based on distance.
  float reflect_dist_mix =
      (reflect_dist - reflect_fade_start) /
      (reflect_fade_end - reflect_fade_start);
  reflect_dist_mix = 1.0 - max(0.0, min(1.0, reflect_dist_mix));

  // TODO: reduce reflection the further we get from the "surface".
  vec4 c = mix(colour, reflect, reflect_mix * reflect_dist_mix);
  c = mix(refract, c, colour.a);
  gl_FragColor = vec4(c.r, c.g, c.b, 1.0);
}
