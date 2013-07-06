#include "pixels.glsl"

attribute vec2 pixels;
attribute vec2 origin;
attribute float range;

varying vec2 pixels_coord;
varying vec2 origin_coord;
varying vec2 pos_coord;
varying float range_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  gl_Position = pos;

  pixels_coord = pixels;
  origin_coord = origin;
  pos_coord = 0.5 + 0.5 * vec2(pos.x, pos.y);

  range_coord = range;
}
