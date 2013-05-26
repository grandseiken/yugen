#include "pixels.glsl"

uniform vec2 origin;
varying vec2 tex_coord;

void main()
{
  gl_Position = pos_from_pixels();
  tex_coord = tex_from_pixels(origin);
}
