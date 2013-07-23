uniform sampler2D normalbuffer;
uniform float depth;

// TODO: profile this and see if this is actually faster than using uniforms.
varying vec2 pixels_coord;
varying vec2 pos_coord;
varying vec2 range_coord;
varying vec4 colour_coord;
varying float layer_coord;

#include "light_util.glsl"

// TODO: make sure this really is correct for planar lights and then rename it
// to get rid of the point_ prefix.
void main()
{
  // Add full range and fall-off range.
  float max_range = range_coord.x + range_coord.y;
  vec2 dist_v = pixels_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float max_range_sq = max_range * max_range;

  if (dist_sq > max_range_sq) {
    discard;
  }

  // Convert texture normal values to world normal values./ Normal has x, y
  // in [0, 1], scale and transform to world normal. Dicard early if the
  // light doesn't affect the layering component (or it hasn't been written to).
  vec4 normal_tex = texture2D(normalbuffer, pos_coord);
  if (layering_value_skip(normal_tex.b, layer_coord)) {
    discard;
  }
  vec3 normal_world = tex_to_world_normal(normal_tex);

  // The correct (mathematically) formula gives extreme results since the light
  // source is in the same 2D plane as the geometry, but the normals are mostly
  // pointing out of the screen. It does give nice contrasting shadows though.
  // To fix this, we mix in some indirect lighting, which is approximated by
  // pretending the light is some distance out of the screen plane, shining
  // towards the screen; i.e., it is a spherical light whose intersection with
  // the screen plane gives the same circle as the direct lighting.
  float dist = sqrt(dist_sq);
  vec2 direct_dir =
      dist_v == vec2(0.0) ? vec2(0.0) : dist_v / dist;
  vec2 indirect_dir = dist_v / max_range;

  // Similarly, light directions have x, y in [-1, 1]; scale and treat as
  // spherical coordinates.
  vec3 direct_world = vec3(direct_dir.x, direct_dir.y, 0.0);
  vec3 indirect_world = circular_coords_to_world_normal(indirect_dir);

  // Calculate light values.
  float direct_light = light_value(direct_world, normal_world);
  float indirect_light = light_value(indirect_world, normal_world);
  float total_light = mix(max(0.0, indirect_light), max(0.0, direct_light),
                          direct_coefficient);

  // Calculate intensity at this point.
  vec4 colour = colour_coord;
  colour.a *= total_light *
      light_range_coefficient(dist, range_coord) *
      layering_value_coefficient(normal_tex.b, layer_coord);
  gl_FragColor = colour;

  // Set the depth to avoid overlapping triangles.
  gl_FragDepth = depth;
}
