#include "pixels.glsl"

uniform ivec3 perlin_size;
uniform vec2 origin;
uniform vec2 tex_offset;
attribute vec2 pixels;
varying vec2 tex_coord;

void main()
{
  tex_coord = 
      (tex_offset + pixels) /
      vec2(perlin_size.x, perlin_size.y);
  gl_Position = pos_from_pixels(pixels);
}
