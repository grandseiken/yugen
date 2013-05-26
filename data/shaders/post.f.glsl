uniform sampler2D framebuffer;
uniform sampler2D bayer;
uniform ivec2 native_res;
uniform ivec2 bayer_res;
uniform vec2 bayer_off;
uniform int bayer_frame;
varying vec2 tex_coord;

const int colours_per_channel = 8;
const float div = 1.0 / (colours_per_channel - 1);
const float pi = 3.1415926536;
// Make sure no direction aligns exactly with an axis.
const vec2 r_dir = vec2(sin(0.2), cos(0.2));
const vec2 g_dir = vec2(sin(0.2 + 2 * pi / 3), cos(0.2 + 2 * pi / 3));
const vec2 b_dir = vec2(sin(0.2 + 4 * pi / 3), cos(0.2 + 4 * pi / 3));

void main()
{
  vec2 bayer_div = vec2(1.0 / bayer_res.x, 1.0 / bayer_res.y);
  vec2 r_off = 0.07 * bayer_frame * bayer_div * r_dir;
  vec2 g_off = 0.11 * bayer_frame * bayer_div * g_dir;
  vec2 b_off = 0.17 * bayer_frame * bayer_div * b_dir;

  vec2 off = bayer_off + 0.5;
  off = off - mod(off, 1.0);
  vec2 bayer_coord =
      (tex_coord * vec2(native_res) - off) / bayer_res;
  vec3 bayer_val =
      vec3(texture2D(bayer, bayer_coord + r_off).x,
           texture2D(bayer, bayer_coord + g_off).x,
           texture2D(bayer, bayer_coord + b_off).x) * div;

  vec4 raw = texture2D(framebuffer, tex_coord);
  vec4 adjusted = raw + vec4(bayer_val, 0.0);
  gl_FragColor = adjusted - mod(adjusted, div);
}
