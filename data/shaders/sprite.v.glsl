#include "pixels.glsl"

attribute vec2 origin;
attribute vec2 frame_index;
varying vec2 tex_coord;
varying vec2 frame_coord;

void main()
{
  gl_Position = pos_from_pixels();
  tex_coord = tex_from_pixels(origin);
  frame_coord = frame_index;
}
