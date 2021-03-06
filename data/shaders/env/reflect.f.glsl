uniform vec4 colour;
uniform ivec3 perlin_size;
uniform ivec2 resolution;
uniform sampler3D perlin;
uniform sampler2D source;
uniform float frame;
uniform float reflect_mix;
uniform float light_passthrough;
uniform float normal_scaling_reflect;
uniform float normal_scaling_refract;
uniform float reflect_fade_start;
uniform float reflect_fade_end;
uniform float wave_height;
uniform float wave_scale;

noperspective varying vec2 tex_coord;
noperspective varying vec2 reflect_coord;
noperspective varying vec2 refract_coord;
noperspective varying float reflect_dist;

#include "../perlin.glsl"

// Sampling outside the source framebuffer produces stretched column artefacts
// in the reflection. Increasing the source framebuffer size helps, but in
// general the reflect fade can always been lengthened such that we start
// sampling outside.
// We solve this by interpolating away the reflection as we start sampling
// towards the edge of the source framebuffer.
const float source_edge_fade_dist = 0.2;

void main()
{
  // Skip to form waves.
  vec2 wave = vec2(perlin_lookup(perlin, vec2(0, wave_scale * tex_coord.x),
                                 frame / perlin_size.z));
  if (reflect_dist < wave_height * wave.x) {
    discard;
  }

  // Calculate texture offset based on perlin noise.
  vec2 p = perlin_lookup(perlin, tex_coord, frame / perlin_size.z);
  p = 2.0 * (p - 0.5);
  p /= resolution;

  // Calculate reflect and refract coordinates.
  vec2 reflect_v = reflect_coord + normal_scaling_reflect * reflect_dist * p;
  vec2 refract_v = refract_coord - normal_scaling_refract * p;

  // Lookup reflected and refracted. Reflected is the colour taken from the
  // point opposite the reflection axis (like sky mirrored in water), whereas
  // refracted is the colour coming through from behind.
  vec3 reflect = texture2D(source, reflect_v);
  vec3 refract = texture2D(source, refract_v);

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

  // Reflections like water are typically drawn on a FULLBRIGHT layer, so the
  // main lighting effects come from the environment behind the water. In order
  // to prevent such water being unnaturally bright, we modulate the colour by
  // the overall brightness behind (but only as much as the refraction is mixed
  // in).
  float refract_brightness = (refract.r + refract.g + refract.b) / 3.0;
  vec3 colour_modulated = mix(colour, refract_brightness * colour,
                              light_passthrough);

  // Mix everything together.
  vec3 c = mix(colour_modulated, reflect,
               reflect_mix * reflect_dist_mix * edge_fade);
  c = mix(refract, c, colour.a);
  gl_FragColor = vec4(c, 1.0);
}
