uniform sampler2D normalbuffer;
uniform float depth;
uniform ivec2 resolution;
uniform vec2 translation;

noperspective varying vec2 pixels_coord;
flat varying vec2 origin_coord;
noperspective varying vec2 pos_coord;
flat varying vec2 range_coord;
flat varying vec4 colour_coord;
flat varying float layer_coord;

#include "light_util.glsl"

// TODO: specular lighting with colours; somehow allow custom specular powers.
// In particular we should allow metallic speculars (colour of material rather
// than always white).
const float specular_power = 2;

void main()
{
  // See light.f.glsl for more detailed comments.
  float max_range = range_coord.x + range_coord.y;
  vec2 dist_v = pixels_coord - origin_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float max_range_sq = max_range * max_range;

  if (dist_sq > max_range_sq) {
    discard;
  }

  // Convert texture normal values to world normal values.
  vec4 normal_tex = texture2D(normalbuffer, pos_coord);
  if (layering_value_skip(normal_tex.b, layer_coord)) {
    discard;
  }
  vec3 normal_world = tex_to_world_normal(normal_tex);

  // Camera position for specular highlights. Distance from render-plane is
  // essentially made-up.
  vec3 camera_pos = vec3(resolution.xy / 2 - translation.xy,
                         camera_distance * resolution.y);

  vec3 camera_to_pixel = vec3(pixels_coord.xy, 0.0) - camera_pos;
  camera_to_pixel = normalize(camera_to_pixel);
  vec3 indirect_light_to_pixel =
      normalize(vec3(dist_v.xy, -max_range));
  vec3 direct_light_to_pixel = normalize(vec3(dist_v.xy, 0.0));

  // Calculate specular values.
  float direct_specular = specular_value(
      direct_light_to_pixel, camera_to_pixel, normal_world);
  float indirect_specular = specular_value(
      indirect_light_to_pixel, camera_to_pixel, normal_world);
  float total_specular = mix(
      specular_intensity(indirect_specular, specular_power),
      specular_intensity(direct_specular, specular_power),
      specular_direct_coefficient);

  vec4 colour = vec4(1.0, 1.0, 1.0, colour_coord.a);
  colour.a *= total_specular *
      light_range_coefficient(sqrt(dist_sq), range_coord) *
      layering_value_coefficient(normal_tex.b, layer_coord);
  gl_FragColor = colour;

  // Set the depth to avoid overlapping triangles.
  gl_FragDepth = depth;
}
