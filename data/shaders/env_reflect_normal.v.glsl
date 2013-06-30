#include "env.glsl"

varying vec2 tex_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  tex_coord = env_tex_coord(pixels);
  gl_Position = pos;
}
