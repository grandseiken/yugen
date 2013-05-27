#include "pixels.glsl"

attribute vec2 pixels;
attribute vec2 origin;
attribute vec2 frame_index;
attribute vec4 colour;
attribute float rotation;
attribute float depth;
uniform ivec2 frame_size;

varying vec2 tex_coord;
varying vec2 frame_coord;
varying vec4 colour_coord;
varying float depth_coord;

void main()
{
  mat2 rot = mat2(cos(rotation), sin(rotation),
                  -sin(rotation), cos(rotation));

  gl_Position = pos_from_pixels(
      (rot * pixels) + frame_size / 2 + origin);
  tex_coord = pixels + frame_size / 2;

  frame_coord = frame_index;
  colour_coord = colour;
  depth_coord = depth;
}
