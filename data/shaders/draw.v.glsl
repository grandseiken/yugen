#include "pixels.glsl"

attribute vec2 frame_index;

void main()
{
  gl_Position = pos_from_pixels();
}
