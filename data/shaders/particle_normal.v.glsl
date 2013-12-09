#include "pixels.glsl"

attribute vec2 pixels;
attribute float depth;

flat varying float depth_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  gl_Position = pos;
  depth_coord = depth;
}
