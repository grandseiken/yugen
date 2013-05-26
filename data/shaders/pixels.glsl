attribute vec2 pixels;
uniform ivec2 resolution;
uniform vec2 translation;
uniform vec2 scale;

// Fragment centre correction.
const float correction = 0.375;

vec4 pos_from_pixels()
{
  vec2 transform = scale * pixels + translation + 0.5;
  transform = transform - mod(transform, 1.0);
  vec2 v = 2.0 * (transform + correction) /
      vec2(resolution) - 1.0;
  return vec4(v.x, -v.y, 0.0, 1.0);
}

vec2 tex_from_pixels(vec2 origin)
{
  return pixels - origin;
}
