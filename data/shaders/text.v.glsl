#include "pixels.glsl"

attribute vec2 pixels;
attribute float character;
uniform vec2 origin;
noperspective varying vec2 tex_coord;
flat varying float character_coord;

void main()
{
  gl_Position = pos_from_pixels(pixels);
  tex_coord = pixels - origin;
  character_coord = character;
}
