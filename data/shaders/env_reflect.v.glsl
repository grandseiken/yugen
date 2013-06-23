#include "env.glsl"

uniform bool flip_x;
uniform bool flip_y;
uniform vec2 flip_axes;

varying vec2 tex_coord;
varying vec2 reflect_coord;
varying vec2 refract_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  vec2 flip = vec2(pos_from_pixels(flip_axes));

  vec2 ref = vec2(flip_x ? 2 * flip.x - pos.x : pos.x,
                  flip_y ? 2 * flip.y - pos.y : pos.y);
  ref = (vec2(1.0, 1.0) + ref) / 2.0;

  tex_coord = env_tex_coord(pixels);
  reflect_coord = ref;
  refract_coord = (vec2(1.0, 1.0) + vec2(pos)) / 2.0;
  gl_Position = pos;
}
