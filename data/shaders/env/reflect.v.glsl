#include "common.glsl"

uniform bool flip_x;
uniform bool flip_y;
uniform vec2 flip_axes;

noperspective varying vec2 tex_coord;
noperspective varying vec2 reflect_coord;
noperspective varying vec2 refract_coord;
noperspective varying float reflect_dist;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  vec2 flip = vec2(pos_from_pixels(flip_axes));

  vec2 ref = vec2(flip_x ? 2 * flip.x - pos.x : pos.x,
                  flip_y ? 2 * flip.y - pos.y : pos.y);
  ref = (1.0 + ref) / 2.0;

  reflect_dist = length(vec2(flip_x ? pixels.x - flip_axes.x : 0.0,
                             flip_y ? pixels.y - flip_axes.y : 0.0));

  tex_coord = env_tex_coord(pixels);
  reflect_coord = ref;
  refract_coord = (1.0 + vec2(pos)) / 2.0;
  gl_Position = pos;
}
