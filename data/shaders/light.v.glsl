#include "pixels.glsl"

attribute vec2 pixels;
attribute vec2 origin;
attribute float range;
attribute float intensity;
varying vec2 pixels_coord;
varying vec2 origin_coord;
varying float range_coord;
varying float intensity_coord;

void main()
{
  gl_Position = pos_from_pixels(pixels);
  pixels_coord = pixels;
  origin_coord = origin;
  range_coord = range;
  intensity_coord = intensity;
}
