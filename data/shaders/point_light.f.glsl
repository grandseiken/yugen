uniform sampler2D normalbuffer;

// TODO: profile this and see if this is actually faster than using uniforms.
varying vec2 pixels_coord;
varying vec2 pos_coord;
varying vec2 range_coord;
varying vec4 colour_coord;

#include "light_util.glsl"

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

  // The correct (mathematically) formula gives extreme results since the light
  // source is in the same 2D plane as the geometry, but the normals are mostly
  // pointing out of the screen. It does give nice contrasting shadows though.
  // To fix this, we mix in some indirect lighting, which is approximated by
  // pretending the light is some distance out of the screen plane, shining
  // towards the screen; i.e., it is a spherical light whose intersection with
  // the screen plane gives the same circle as the direct lighting.
  vec2 direct_dir =
      dist_v == vec2(0.0) ? vec2(0.0) : dist_v * inversesqrt(dist_sq);
  vec2 indirect_dir = dist_v / max_range;

  // Convert texture normal values to world normal values.
  // Normal has x, y in [0, 1], scale and transform to world normal.
  // TODO: is it possible to discard early if the normalbuffer was never written
  // to, somehow? In fact, if we use green component for layering, as discussed
  // below, it should definitely be possible by discaring when it's zero, for
  // instance.
  vec4 normal_tex = texture2D(normalbuffer, pos_coord);
  vec3 normal_world = tex_to_world_normal(normal_tex);

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
  // TODO: need some layering mechanism for when lights work, stored in green
  // component of normal, perhaps, for example so that lights inside water
  // don't add specular. But - how do lights, individually, know what they
  // affect?
  vec4 colour = colour_coord;
  colour.a *= total_light * light_range_coefficient(dist_sq, range_coord);
  gl_FragColor = colour;
}
