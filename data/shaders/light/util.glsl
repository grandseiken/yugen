const float cel_shade_clamp = 1.0 / 4;
const float cel_shade_clamp_specular = 1.0 / 3;
const float cel_shade_mix = 0.75;

#include "../gamma.glsl"
const float ambient_light =
    use_gamma_correction ? 0.2 : 0.05;
const bool ambient_is_post_cel_shade = true;

const float direct_coefficient = 0.5;
const float specular_direct_coefficient = 0.5;
const float camera_distance = 0.1;

// Distance over which to interpolate the disappearance of light due to layering
// value.
const float layering_fade_distance = 0.05;

// Given coords in [-1, 1] X [-1, 1], returns vector v such that v.x and v.y
// are the coords scaled to the unit circle, v.z is positive, and v has
// length 1.
vec3 sq_coords_to_world_normal(vec2 coords)
{
  vec2 sq = coords * coords;
  float m = max(sq.x, sq.y);
  float d = inversesqrt((sq.x + sq.y) / m);
  return vec3(coords.xy * d, sqrt(max(0.0, 1.0 - m)));
}

// Same as above, but takes coords scaled to the unit circle already.
vec3 circular_coords_to_world_normal(vec2 coords)
{
  vec2 sq = coords * coords;
  return vec3(coords.xy, sqrt(max(0.0, 1 - sq.x - sq.y)));
}

// Returns the square cooords of a normal encoded in a texture value.
vec2 tex_to_sq_coords(vec4 normal_tex)
{
  return 2.0 * normal_tex.xy - 1.0;
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
             vec3(-light_normal.xy, light_normal.z));
}

// Calculates the specular value given normals from light to pixel, camera to
// pixel, and surface.
float specular_value(vec3 light_normal, vec3 camera_normal, vec3 world_normal)
{
  return dot(reflect(light_normal, world_normal), -camera_normal);
}

// Calculates the light intensity multiplier based on full range and fall-off
// range. When full range is zero, reduces to 1 - dist_sq / falloff_range_sq.
float light_range_coefficient(
    float dist, vec2 full_and_falloff_range)
{

  float full_range = full_and_falloff_range.x;
  float falloff_range = full_and_falloff_range.y;

  if (dist < full_range) {
    return 1.0;
  }
  float d = dist - full_range;
  return 1.0 - (d * d) / (falloff_range * falloff_range);
}

float specular_intensity(float value, float power)
{
  float d = max(value, 0);
  return pow(d, power);
}

// Layering skip.
bool layering_value_skip(float target_layer, float light_layer)
{
  return target_layer <= 0.0 ||
      target_layer + layering_fade_distance < light_layer;
}

// Layering coefficient.
float layering_value_coefficient(float target_layer, float light_layer)
{
  float d = (light_layer - target_layer) / layering_fade_distance;
  // Don't need to take max(0.0, 1.0 - d) since skip function above ensures that
  // d <= 1.0.
  return min(1.0, 1.0 - d);
}

// Implements a kind of cel-shading based on the constants above.
float cel_shade(float light, bool ambient, bool specular)
{
  if (ambient && !ambient_is_post_cel_shade) {
    light = light + ambient_light;
  }

  float clamp = specular ? cel_shade_clamp_specular : cel_shade_clamp;
  float cel = light + 0.5 * clamp;
  cel = cel - mod(cel, clamp);
  light = mix(light, cel, cel_shade_mix);

  if (ambient && ambient_is_post_cel_shade) {
    light = light + ambient_light;
  }

  return min(light, 1.0);
}

vec3 cel_shade(vec3 light, bool ambient, bool specular)
{
  return vec3(cel_shade(light.r, ambient, specular),
              cel_shade(light.g, ambient, specular),
              cel_shade(light.b, ambient, specular));
}
