#version 120

uniform ivec2 resolution;
uniform ivec2 origin;
attribute vec2 pixels;
varying vec2 tex_coord;

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
  gl_Position = pos_from_pixels(pixels);
  tex_coord = tex_from_pixels(pixels);
}
