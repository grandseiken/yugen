#include "pixels.glsl"

uniform ivec3 perlin_size;
uniform vec2 tex_offset;
attribute vec2 pixels;

vec2 env_tex_coord(vec2 pixels)
{
  return (tex_offset + pixels) / vec2(perlin_size.x, perlin_size.y);
}
