#include "common.glsl"

noperspective varying vec2 tex_coord;

void main()
{
  tex_coord = env_tex_coord(pixels);
  gl_Position = pos_from_pixels(pixels);
}
