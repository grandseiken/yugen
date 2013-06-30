uniform sampler2D normalbuffer;
uniform ivec2 resolution;
uniform vec2 translation;

varying vec2 pixels_coord;
varying vec2 origin_coord;
varying vec2 pos_coord;
varying float range_coord;

// TODO: specular lighting.
// Possible strategies!
// A: in normal-map, store x-normal, y-normal and specular coefficient.
// Associate everything else with a tileset.
// B: in normal-map, store x-normal, y-normal and specular power.
// In another coefficient map, store the coefficients of direct, indirect
// and specular lighting.
// More generally, possibly specular should be an explicit extra layer,
// or something like that?

#include "light_util.glsl"

void main()
{
  vec2 dist_v = pixels_coord - origin_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float range_sq = range_coord * range_coord;

  if (dist_sq > range_sq) {
    discard;
  }

  // Convert texture normal values to world normal values.
  // Normal has x, y in [0, 1], scale and transform to world normal.
  vec4 normal_tex = texture2D(normalbuffer, pos_coord);
  vec3 normal_world = tex_to_world_normal(normal_tex);

  // Camera position for specular highlights. Distance from render-plane is
  // essentially made-up.
  vec3 camera_pos = vec3(resolution.x / 2 - translation.x,
                         resolution.y / 2 - translation.y,
                         camera_distance * resolution.y);

  vec3 camera_to_pixel = vec3(pixels_coord.x, pixels_coord.y, 0.0) - camera_pos;
  camera_to_pixel = normalize(camera_to_pixel);
  vec3 indirect_light_to_pixel =
      normalize(vec3(dist_v.x, dist_v.y, -range_coord));
  vec3 direct_light_to_pixel = normalize(vec3(dist_v.x, dist_v.y, 0.0));

  // Calculate specular values.
  float direct_specular = specular_value(
      direct_light_to_pixel, camera_to_pixel, normal_world);
  float indirect_specular = specular_value(
      indirect_light_to_pixel, camera_to_pixel, normal_world);
  float total_specular = mix(specular_intensity(indirect_specular),
                             specular_intensity(direct_specular),
                             specular_direct_coefficient);

  vec4 colour = vec4(1.0, 1.0, 1.0, 1.0);
  colour.a *= total_specular * (1.0 - dist_sq / range_sq);
  gl_FragColor = colour;
}
