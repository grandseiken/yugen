uniform sampler2D normalbuffer;

varying vec2 pixels_coord;
varying vec2 origin_coord;
varying vec2 pos_coord;
varying float range_coord;
varying float intensity_coord;

// const float ambient_light = 0.1;
const float pi = 3.1415926535897932384626433832795028841971693993751058209749;

void main()
{
  vec2 dist_v = pixels_coord - origin_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float range_sq = range_coord * range_coord;

  if (dist_sq > range_sq) {
    discard;
  }

  float dist = sqrt(dist_sq);
  float range = sqrt(range_sq);

  // Calculate intensity at this point.
  float intensity = intensity_coord * (1.0 - dist_sq / range_sq);

  // Calculate normalised direction vector from light origin to pixel.
  vec2 dir_v = dist == 0 ? vec2(0.0, 0.0) : dist_v / dist;

  // Scale by the distance from the origin.
  vec2 light_dir = dir_v * (dist / range);

  vec4 normal = texture2D(normalbuffer, pos_coord);

  // Convert texture normal values to world normal values.
  // Normal has x, y in [0, 1]; scale to [-pi / 2, pi / 2] and treat as
  // spherical coordinates.
  vec3 normal_world = vec3(
      sin(pi * (normal.x - 0.5)) * cos(pi * (normal.y - 0.5)),
      sin(pi * (normal.y - 0.5)),
      cos(pi * (normal.x - 0.5)) * cos(pi * (normal.y - 0.5)));

  // Similarly, light_dir has x, y in [-1, 1]; scale and treat as spherical.
  vec3 light_dir_world = vec3(
      sin(0.5 * pi * light_dir.x) * cos(0.5 * pi * light_dir.y),
      sin(0.5 * pi * light_dir.y),
      cos(0.5 * pi * light_dir.x) * cos(0.5 * pi * light_dir.y));
  
  // Calculate light value.
  normal_world.x = -normal_world.x;
  normal_world.y = -normal_world.y;
  float light = dot(normal_world, light_dir_world) * intensity;

  gl_FragColor = vec4(1.0, 1.0, 1.0, light);
}
