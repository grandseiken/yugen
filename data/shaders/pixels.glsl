attribute vec2 pixels;
attribute float rotation;
uniform ivec2 resolution;
uniform vec2 translation;
uniform vec2 scale;

// Fragment centre correction.
const float correction = 0.375;

vec4 pos_from_pixels_internal(vec2 p)
{
  vec2 transform = 0.5 + translation + p;
  transform = transform - mod(transform, 1.0);
  vec2 v = 2.0 * (transform + correction) /
      vec2(resolution) - 1.0;
  return vec4(v.x, -v.y, 0.0, 1.0);
}

vec4 pos_from_pixels()
{
  return pos_from_pixels_internal(scale * pixels);
}

vec4 pos_from_pixels_rotation(vec2 offset)
{
  mat2 rot = mat2(cos(rotation), sin(rotation),
                  -sin(rotation), cos(rotation));

  return pos_from_pixels_internal(
      scale * (offset + (rot * (pixels - offset))));
}

vec2 tex_from_pixels(vec2 origin)
{
  return pixels - origin;
}
