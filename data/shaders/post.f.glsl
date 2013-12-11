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

// Whether dithering moves around (based on per-colour directions above), and
// whether it is separated by colour.
const bool dithering_move = true;
const bool dithering_monochrome = false;
// How much to mix in dithering versus true colouring.
const float dithering_mix = 1.;
// See also http://bisqwit.iki.fi/story/howto/dither/jy/ for the dithering
// bible.
vec3 make_coord(vec2 coord, vec2 c_off, float c)
{
  if (dithering_monochrome) {
    return vec3(coord / dither_res, 0);
  }
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

vec3 floor_div(vec3 v)
{
  return v - mod(v, div);
}

vec3 linear_dither(vec3 raw, vec3 dither_val)
{
  vec3 adjusted = raw +
      mix(vec3(0.5 * div), dither_val * div, dithering_mix);
  return floor_div(adjusted);
}

vec3 gamma_correct_dither(vec3 raw, vec3 dither_val)
{
  vec3 a = floor_div(raw);
  vec3 b = a + div;
  vec3 correct_ratio = 1 -
      ((gamma_write(b) - gamma_write(raw)) /
       (gamma_write(b) - gamma_write(a)));
  return floor_div(a + (correct_ratio + dither_val) * div);
}

void main()
{
  vec3 raw = vec3(texture2D(framebuffer, tex_coord));

  vec2 r_off = 0.05 * r_dir * dither_frame;
  vec2 g_off = 0.07 * g_dir * dither_frame;
  vec2 b_off = 0.11 * b_dir * dither_frame;
  vec3 dither_val = matrix_lookup(dither_coord, r_off, g_off, b_off);

  gl_FragColor = vec4(gamma_correct_dither(raw, dither_val), 1.0);
}
