#include "pixels.glsl"

attribute vec2 pixels;
attribute float depth;
attribute float layering_value;

flat varying float depth_coord;
flat varying float layering_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  gl_Position = pos;
  depth_coord = depth;
  layering_coord = layering_value;
}
