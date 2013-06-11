uniform sampler2D normalbuffer;

varying vec2 pixels_coord;
varying vec2 origin_coord;
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
const float direct_coefficient = 0.6;
const float indirect_coefficient = 0.4;

// Given coords in [-1, 1] X [-1, 1], returns vector v such that v.x and v.y
// are the coords scaled to the unit circle, v.z is positive, and v has
// length 1.
vec3 sq_coords_to_world_normal(vec2 coords)
{
  vec2 sq = coords * coords;
  float m = max(sq.x, sq.y);
  float d = inversesqrt((sq.x + sq.y) / m);
  return vec3(coords.x * d, coords.y * d, sqrt(1 - m));
}

// Same as above, but takes coords scaled to the unit circle already.
vec3 circular_coords_to_world_normal(vec2 coords)
{
  vec2 sq = coords * coords;
  return vec3(coords.x, coords.y, sqrt(1 - sq.x - sq.y));
}

void main()
{
  vec2 dist_v = pixels_coord - origin_coord;
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
  vec3 normal_world =
      sq_coords_to_world_normal(2.0 * vec2(normal_tex.x, normal_tex.y) - 1.0);

  // Similarly, light directions have x, y in [-1, 1]; scale and treat as
  // spherical coordinates.
  vec3 direct_world = circular_coords_to_world_normal(direct_dir);
  vec3 indirect_world = circular_coords_to_world_normal(indirect_dir);
  
  // Calculate light value.
  normal_world.x = -normal_world.x;
  normal_world.y = -normal_world.y;
  float direct_light = dot(normal_world, direct_world);
  float indirect_light = dot(normal_world, indirect_world);
  float total_light = direct_coefficient * max(0.0, direct_light) +
                      indirect_coefficient * max(0.0, indirect_light);

  // Calculate intensity at this point.
  vec4 colour = colour_coord;
  colour.a *= total_light * (1.0 - dist_sq / range_sq);
  gl_FragColor = colour;
}
