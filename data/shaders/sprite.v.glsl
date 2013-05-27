#include "pixels.glsl"

attribute vec2 frame_index;
attribute vec4 colour;
attribute float depth;
attribute vec2 origin;
uniform ivec2 frame_size;
varying vec2 tex_coord;
varying vec2 frame_coord;
varying vec4 colour_coord;
varying float depth_coord;

void main()
{
  gl_Position = pos_from_pixels_rotation(origin + frame_size / 2);
  tex_coord = tex_from_pixels(origin);
  frame_coord = frame_index;
  colour_coord = colour;
  depth_coord = depth;
}
