uniform sampler2D framebuffer;
uniform sampler3D dither_matrix;
uniform ivec2 native_res;
uniform ivec2 dither_res;
uniform vec2 dither_off;
uniform float dither_rot;
uniform int dither_frame;
noperspective varying vec2 tex_coord;

const int colours_per_channel = 12;
const float div = 1.0 / (colours_per_channel - 1);
const float pi = 3.1415926536;
// Make sure no direction aligns exactly with an axis.
const vec2 r_dir = vec2(sin(0.2), cos(0.2));
const vec2 g_dir = vec2(sin(0.2 + 2 * pi / 3), cos(0.2 + 2 * pi / 3));
const vec2 b_dir = vec2(sin(0.2 + 4 * pi / 3), cos(0.2 + 4 * pi / 3));

const bool dithering_move = true;
const float dithering_mix = 1.0;
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
  vec4 raw = texture2D(framebuffer, tex_coord);

  vec2 r_off = 0.05 * r_dir * dither_frame;
  vec2 g_off = 0.07 * g_dir * dither_frame;
  vec2 b_off = 0.11 * b_dir * dither_frame;

  // We attempt to preserve the dithering so that it looks right when the
  // camera moves, rather than being an obvious overlay.
  // However, it isn't possible to provide this consistently for both movement
  // and rotation: the dithering will appear to shift relative to the world
  // either when the camera rotates or when a rotated camera moves. (This
  // depends on the presence of the rot multiplier.) It seems like moving a
  // rotated camera is the common case, and the dithering going mad when
  // rotating looks a bit less weird.
  // TODO: it must be possible to do this properly.
  vec2 off = vec2(dither_off.x, -dither_off.y) + 0.5;
  off = off - mod(off, 1.0);
  mat2 rot = mat2(cos(dither_rot), sin(dither_rot),
                  -sin(dither_rot), cos(dither_rot));

  vec2 dither_coord = tex_coord * vec2(native_res) - rot * off;
  vec3 dither_val = matrix_lookup(dither_coord, r_off, g_off, b_off);

  vec4 adjusted = raw +
      mix(vec4(0.5 * div), vec4(dither_val * div, 0.0), dithering_mix);
  gl_FragColor = adjusted - mod(adjusted, div);
}
