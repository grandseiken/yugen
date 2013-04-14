#version 120

uniform ivec2 resolution;
attribute vec2 pixels;
attribute vec2 origin;
attribute vec2 frame_index;
varying vec2 tex_coord;
varying vec2 frame_coord;

vec4 pos_from_pixels(vec2 position)
{
  vec2 v = 2.0 * position / vec2(resolution) - 1.0;
  return vec4(v.x, -v.y, 0.0, 1.0);
}

vec2 tex_from_pixels(vec2 position)
{
  return position - vec2(origin);
}

void main()
{
  // Fragment centre correction.
  vec2 pixels_corrected = pixels + 0.375;

  gl_Position = pos_from_pixels(pixels_corrected);
  tex_coord = tex_from_pixels(pixels_corrected);
  frame_coord = frame_index;
}
