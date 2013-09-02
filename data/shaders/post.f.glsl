uniform sampler2D framebuffer;
uniform sampler2D bayer;
uniform ivec2 native_res;
uniform ivec2 bayer_res;
uniform vec2 dither_off;
uniform float dither_rot;
uniform int dither_frame;
noperspective varying vec2 tex_coord;

const int colours_per_channel = 16;
const float div = 1.0 / (colours_per_channel - 1);
const float pi = 3.1415926536;
// Make sure no direction aligns exactly with an axis.
const vec2 r_dir = vec2(sin(0.2), cos(0.2));
const vec2 g_dir = vec2(sin(0.2 + 2 * pi / 3), cos(0.2 + 2 * pi / 3));
const vec2 b_dir = vec2(sin(0.2 + 4 * pi / 3), cos(0.2 + 4 * pi / 3));

const float dithering_mix = 1.0;
// Use "a dither" algorithm based on http://pippin.gimp.org/a_dither.
// Implemented in hardware algorithm; should use a lookup-table as with
// ordered bayer (which is used when this is false), since this is buggy
// due to int-float conversion (I think).
//
// See also http://bisqwit.iki.fi/story/howto/dither/jy/ for the dithering
// bible.
const bool a_dither = false;

vec3 bayer_lookup(vec2 coord, vec2 r, vec2 g, vec2 b)
{
  vec2 bayer_div = 1.0 / vec2(bayer_res);
  return
      vec3(texture2D(bayer, (coord + r) * bayer_div).x,
           texture2D(bayer, (coord + g) * bayer_div).x,
           texture2D(bayer, (coord + b) * bayer_div).x);
}

const int a_dither_pattern = 3;
float a_dither_lookup(vec2 coord, int c)
{
  int x = int(0.5 + coord.x) - (coord.x < 0 ? 1 : 0);
  int y = int(0.5 + coord.y) - (coord.y < 0 ? 1 : 0);
  return
      a_dither_pattern == 0 ?
          ((x ^ y * 149) * 1234 & 511) / 511.0 :
      a_dither_pattern == 1 ?
          (((x + c * 17) ^ y * 149) * 1234 & 511) / 511.0 :
      a_dither_pattern == 2 ?
          ((x + y * 237) * 119 & 255) / 255.0 :
      a_dither_pattern == 3 ?
          (((x + c * 67) + y * 236) * 119 & 255) / 255.0 : 0.5;
}

vec3 a_dither_lookup(vec2 coord, vec2 r, vec2 g, vec2 b)
{
  return
      vec3(a_dither_lookup(coord + r, 0),
           a_dither_lookup(coord + g, 1),
           a_dither_lookup(coord + b, 2));
}

void main()
{
  vec4 raw = texture2D(framebuffer, tex_coord);

  vec2 r_off = 0.05 * r_dir * dither_frame;
  vec2 g_off = 0.07 * g_dir * dither_frame;
  vec2 b_off = 0.11 * b_dir * dither_frame;

  // To preserve the straightness of the dithering, the dithering must be
  // shifted either when rotating or when moving a rotated camera. This
  // is controlled by the presence of the rot multiplier. Not sure which
  // will be the most common case to optimise for, but right now the dithering
  // going a bit mad when rotating looks less weird.
  // (This depends on order of operations of the rotate and translate.)
  vec2 off = vec2(dither_off.x, -dither_off.y) + 0.5;
  off = off - mod(off, 1.0);
  mat2 rot = mat2(cos(dither_rot), sin(dither_rot),
                  -sin(dither_rot), cos(dither_rot));

  vec2 dither_coord = tex_coord * vec2(native_res) - rot * off;
  vec3 dither_val = a_dither ?
      a_dither_lookup(dither_coord, r_off, g_off, b_off) :
      bayer_lookup(dither_coord, r_off, g_off, b_off);

  vec4 adjusted = raw +
      mix(vec4(0.5 * div), vec4(dither_val * div, 0.0), dithering_mix);
  gl_FragColor = adjusted - mod(adjusted, div);
}
