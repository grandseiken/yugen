#include "pixels.glsl"

uniform ivec3 perlin_size;
uniform int frame;
attribute vec2 pixels;
varying vec2 tex_coord;

void main()
{
  tex_coord = (vec2(-frame / 4.0, frame / 8.0) + pixels) /
      vec2(perlin_size.x, perlin_size.y);
  gl_Position = pos_from_pixels(pixels);
}
