attribute vec2 pixels;
uniform ivec2 resolution;

// Fragment centre correction.
const float correction = 0.375;

vec4 pos_from_pixels()
{
  vec2 v = 2.0 * (pixels + correction) / vec2(resolution) - 1.0;
  return vec4(v.x, -v.y, 0.0, 1.0);
}

vec2 tex_from_pixels(vec2 origin)
{
  return pixels + correction - origin;
}