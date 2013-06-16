#include "pixels.glsl"

attribute vec2 pixels;

void main()
{
  gl_Position = pos_from_pixels(pixels);
}
