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

// Returns the square cooords of a normal encoded in a texture value.
vec2 tex_to_sq_coords(vec4 normal_tex)
{
  return 2.0 * vec2(normal_tex.x, normal_tex.y) - 1.0;
}

// Returns the world normal from its encoding in a texture value.
vec3 tex_to_world_normal(vec4 normal_tex)
{
  return sq_coords_to_world_normal(tex_to_sq_coords(normal_tex));
}

// Calculates light value from light normal and world normal.
float light_value(vec3 light_normal, vec3 world_normal)
{
  // Since light_normal has reversed z, we only negate the first two
  // coordinates.
  return dot(world_normal,
             vec3(-light_normal.x, -light_normal.y, light_normal.z));
}

const float cel_shade_clamp = 1.0 / 4;
const float cel_shade_mix = 0.75;

const float ambient_light = 0.05;
const bool ambient_is_post_cel_shade = true;

// Implements a kind of cel-shading based on the constants above.
float cel_shade(float light)
{
  if (!ambient_is_post_cel_shade) {
    light = light + ambient_light;
  }

  float cel = light + 0.5 * cel_shade_clamp;
  cel = cel - mod(cel, cel_shade_clamp);
  light = cel_shade_mix * cel + (1 - cel_shade_mix) * light;

  if (ambient_is_post_cel_shade) {
    light = light + ambient_light;
  }

  return min(light, 1.0);
}

vec3 cel_shade(vec3 light)
{
  return vec3(cel_shade(light.r), cel_shade(light.g), cel_shade(light.b));
}
