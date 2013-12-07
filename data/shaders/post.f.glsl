uniform sampler2D framebuffer;
uniform sampler3D dither_matrix;
uniform ivec2 dither_res;
uniform int dither_frame;
noperspective varying vec2 tex_coord;
noperspective varying vec2 dither_coord;

#include "gamma.glsl"

const int colours_per_channel = 16;
const float div = 1.0 / (colours_per_channel - 1);
const float pi = 3.1415926536;
// Make sure no direction aligns exactly with an axis.
const vec2 r_dir = vec2(sin(0.2), cos(0.2));
const vec2 g_dir = vec2(sin(0.2 + 2 * pi / 3), cos(0.2 + 2 * pi / 3));
const vec2 b_dir = vec2(sin(0.2 + 4 * pi / 3), cos(0.2 + 4 * pi / 3));

// Whether dithering moves around (based on per-colour directions above).
const bool dithering_move = true;
// How much to mix in dithering versus true colouring.
const float dithering_mix = 1.;
// See also http://bisqwit.iki.fi/story/howto/dither/jy/ for the dithering
// bible.
vec3 make_coord(vec2 coord, vec2 c_off, float c)
{
  return vec3((dithering_move ? coord + c_off :
      coord + vec2(0.001) * dither_frame) / vec2(dither_res), c);
}

vec3 matrix_lookup(vec2 coord, vec2 r, vec2 g, vec2 b)
{
  return
      vec3(texture3D(dither_matrix, make_coord(coord, r, 0.0)).x,
           texture3D(dither_matrix, make_coord(coord, g, 1.0 / 3)).x,
           texture3D(dither_matrix, make_coord(coord, b, 2.0 / 3)).x);
}

void main()
{
  vec4 raw = gamma_write(texture2D(framebuffer, tex_coord));

  vec2 r_off = 0.05 * r_dir * dither_frame;
  vec2 g_off = 0.07 * g_dir * dither_frame;
  vec2 b_off = 0.11 * b_dir * dither_frame;
  vec3 dither_val = matrix_lookup(dither_coord, r_off, g_off, b_off);

  vec4 adjusted = raw +
      mix(vec4(0.5 * div), vec4(dither_val * div, 0.0), dithering_mix);
  gl_FragColor = adjusted - mod(adjusted, div);
}
