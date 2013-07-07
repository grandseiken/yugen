#include "pixels.glsl"

attribute vec2 pixels;
attribute float character;
uniform vec2 origin;
varying vec2 tex_coord;
varying float character_coord;

void main()
{
  gl_Position = pos_from_pixels(pixels);
  tex_coord = pixels - origin;
  character_coord = character;
}
