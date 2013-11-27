#include "pixels.glsl"

attribute vec2 pixels;
attribute vec4 colour;
attribute float depth;

flat varying vec4 colour_coord;
flat varying float depth_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  gl_Position = pos;
  colour_coord = colour;
  depth_coord = depth;
}
