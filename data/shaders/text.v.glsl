#include "pixels.glsl"

attribute vec2 pixels;
uniform vec2 origin;
varying vec2 tex_coord;

void main()
{
  gl_Position = pos_from_pixels(pixels);
  tex_coord = pixels - origin;
}
