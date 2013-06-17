#include "pixels.glsl"

uniform ivec2 perlin_size;
attribute vec2 pixels;
varying vec2 tex_coord;

void main()
{
  tex_coord = pixels / vec2(perlin_size);
  gl_Position = pos_from_pixels(pixels);
}
