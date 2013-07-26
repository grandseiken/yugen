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
uniform float wave_height;
uniform float wave_scale;

varying vec2 tex_coord;
varying vec2 reflect_coord;
varying vec2 refract_coord;
varying float reflect_dist;

#include "perlin.glsl"


// Sampling outside the source framebuffer produces stretched column artefacts
// in the reflection. Increasing the source framebuffer size helps, but in
// general the reflect fade can always been lengthened such that we start
// sampling outside.
// We solve this by interpolating away the reflection as we start sampling
// towards the edge of the source framebuffer.
const float source_edge_fade_dist = 0.2;

// TODO: also refract the light inside the reflection. This requires being
// able to draw shaders to the lightbuffer after the light is rendered.
void main()
{
  // Skip to form waves.
  vec2 wave = vec2(perlin_lookup(perlin, vec2(0, wave_scale * tex_coord.x),
                                 frame / perlin_size.z));
  if (reflect_dist < wave_height * wave.x) {
    discard;
  }

  // Calculate texture offset based on perlin noise.
  vec2 p = vec2(perlin_lookup(perlin, tex_coord, frame / perlin_size.z));
  p = 2.0 * (p - 0.5);
  p /= vec2(resolution);

  // Calculate reflect and refract coordinates.
  vec2 reflect_v = reflect_coord + normal_scaling_reflect * reflect_dist * p;
  vec2 refract_v = refract_coord - normal_scaling_refract * p;

  // Mix reflected, refracted and colour.
  vec4 reflect = texture2D(source, reflect_v);
  vec4 refract = texture2D(source, refract_v);

  // Calculate reflection coefficient based on distance.
  float reflect_dist_mix =
      (reflect_dist - reflect_fade_start) /
      (reflect_fade_end - reflect_fade_start);
  reflect_dist_mix = 1.0 - clamp(reflect_dist_mix, 0.0, 1.0);

  // Interpolate near the edge of source framebuffer.
  float edge_dist = min(
      min(reflect_v.x, reflect_v.y),
      min(1.0 - reflect_v.x, 1.0 - reflect_v.y));
  float edge_fade = clamp(edge_dist / source_edge_fade_dist, 0.0, 1.0);

  vec4 c = mix(colour, reflect, reflect_mix * reflect_dist_mix * edge_fade);
  c = mix(refract, c, colour.a);
  gl_FragColor = vec4(c.r, c.g, c.b, 1.0);
}
