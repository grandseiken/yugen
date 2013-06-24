uniform sampler2D normalbuffer;

// TODO: profile this and see if this is actually faster than using uniforms.
varying vec2 pixels_coord;
varying vec2 pos_coord;
varying float range_coord;
varying vec4 colour_coord;

// TODO: specular lighting.
// Possible strategies!
// A: in normal-map, store x-normal, y-normal and specular coefficient.
// Associate everything else with a tileset.
// B: in normal-map, store x-normal, y-normal and specular power.
// In another coefficient map, store the coefficients of direct, indirect
// and specular lighting.
const float direct_coefficient = 0.5;
const float indirect_coefficient = 1.0 - direct_coefficient;

#include "light_util.glsl"

void main()
{
  vec2 dist_v = pixels_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float range_sq = range_coord * range_coord;

  if (dist_sq > range_sq) {
    discard;
  }
  float range_inv = inversesqrt(range_sq);
  float dist_inv = inversesqrt(dist_sq);

  // The correct (mathematically) formula gives extreme results since the light
  // source is in the same 2D plane as the geometry, but the normals are mostly
  // pointing out of the screen. It does give nice contrasting shadows though.
  // To fix this, we mix in some indirect lighting, which is approximated by
  // pretending the light is some distance out of the screen plane, shining
  // towards the screen; i.e., it is a spherical light whose intersection with
  // the screen plane gives the same circle as the direct lighting.
  vec2 direct_dir =
      dist_v == vec2(0.0) ? vec2(0.0) : dist_v * dist_inv;
  vec2 indirect_dir = dist_v * range_inv;

  // Convert texture normal values to world normal values.
  // Normal has x, y in [0, 1], scale and transform to world normal.
  // TODO: is it possible to discard early if the normalbuffer was never written
  // to, somehow?
  vec4 normal_tex = texture2D(normalbuffer, pos_coord);
  vec3 normal_world = tex_to_world_normal(normal_tex);

  // Similarly, light directions have x, y in [-1, 1]; scale and treat as
  // spherical coordinates.
  vec3 direct_world = circular_coords_to_world_normal(direct_dir);
  vec3 indirect_world = circular_coords_to_world_normal(indirect_dir);

  // Calculate light values.
  float direct_light = light_value(direct_world, normal_world);
  float indirect_light = light_value(indirect_world, normal_world);
  float total_light = direct_coefficient * max(0.0, direct_light) +
                      indirect_coefficient * max(0.0, indirect_light);

  // Calculate intensity at this point.
  vec4 colour = colour_coord;
  colour.a *= total_light * (1.0 - dist_sq / range_sq);
  gl_FragColor = colour;
}
