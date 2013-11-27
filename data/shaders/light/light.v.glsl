#include "../pixels.glsl"

attribute vec2 pixels;
attribute vec2 origin;
attribute vec2 range;
attribute vec4 colour;
attribute float layer;

noperspective varying vec2 pixels_coord;
noperspective varying vec2 pos_coord;
flat varying vec2 range_coord;
flat varying vec4 colour_coord;
flat varying float layer_coord;

void main()
{
  vec4 pos = pos_from_pixels(pixels);
  gl_Position = pos;

  pixels_coord = pixels - origin;
  pos_coord = 0.5 + 0.5 * pos.xy;

  range_coord = range;
  colour_coord = colour;
  layer_coord = layer;
}
