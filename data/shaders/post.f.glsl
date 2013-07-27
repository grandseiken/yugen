uniform sampler2D framebuffer;
uniform sampler2D bayer;
uniform ivec2 native_res;
uniform ivec2 bayer_res;
uniform vec2 bayer_off;
uniform float bayer_rot;
uniform int bayer_frame;
noperspective varying vec2 tex_coord;

const int colours_per_channel = 16;
const float div = 1.0 / (colours_per_channel - 1);
const float pi = 3.1415926536;
// Make sure no direction aligns exactly with an axis.
const vec2 r_dir = vec2(sin(0.2), cos(0.2));
const vec2 g_dir = vec2(sin(0.2 + 2 * pi / 3), cos(0.2 + 2 * pi / 3));
const vec2 b_dir = vec2(sin(0.2 + 4 * pi / 3), cos(0.2 + 4 * pi / 3));

const float dithering_mix = 1.0;

// See http://bisqwit.iki.fi/story/howto/dither/jy/ for the dithering bible.
// TODO: change everything to GLSL 1.3, i.e. in and out rather than attribute
// and varying, and so on. See
// http://www.opengl.org/registry/doc/GLSLangSpec.Full.1.30.10.pdf
void main()
{
  vec4 raw = texture2D(framebuffer, tex_coord);

  vec2 bayer_div = 1.0 / vec2(bayer_res);
  vec2 r_off = 0.05 * bayer_frame * bayer_div * r_dir;
  vec2 g_off = 0.07 * bayer_frame * bayer_div * g_dir;
  vec2 b_off = 0.11 * bayer_frame * bayer_div * b_dir;

  vec2 off = bayer_off + 0.5;
  off = off - mod(off, 1.0);
  mat2 rot = mat2(cos(bayer_rot), sin(bayer_rot),
                  -sin(bayer_rot), cos(bayer_rot));

  vec2 bayer_coord = tex_coord * vec2(native_res);
  // To preserve the straightness of the dithering, the dithering must be
  // shifted either when rotating or when moving a rotated camera. This
  // is controlled by the presence of the rot multiplier. Not sure which
  // will be the most common case to optimise for, but right now the dithering
  // going a bit mad when rotating looks less weird.
  bayer_coord = bayer_coord - rot * off;
  bayer_coord /= bayer_res;
  vec3 bayer_val =
      vec3(texture2D(bayer, bayer_coord + r_off).x,
           texture2D(bayer, bayer_coord + g_off).x,
           texture2D(bayer, bayer_coord + b_off).x) * div;

  vec4 adjusted = raw +
      mix(vec4(0.5 * div), vec4(bayer_val, 0.0), dithering_mix);
  gl_FragColor = adjusted - mod(adjusted, div);
}
