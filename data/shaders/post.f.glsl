uniform sampler2D framebuffer;
uniform sampler2D bayer;
uniform ivec2 native_res;
uniform ivec2 bayer_res;
uniform int bayer_frame;
varying vec2 tex_coord;

const int colours_per_channel = 8;
const float div = 1.0 / (colours_per_channel - 1);
const float sin_third = 0.86602540378;

void main()
{ 
  vec2 bayer_div = vec2(1.0 / bayer_res.x, 1.0 / bayer_res.y);
  vec2 r_off = 0.07 * bayer_frame * bayer_div * vec2(0.0, 1.0);
  vec2 g_off = 0.11 * bayer_frame * bayer_div * vec2(sin_third, -0.5);
  vec2 b_off = 0.17 * bayer_frame * bayer_div * vec2(-sin_third, -0.5);

  vec2 bayer_coord = tex_coord * vec2(native_res) / bayer_res;
  vec3 bayer_val =
      vec3(texture2D(bayer, bayer_coord + r_off).x,
           texture2D(bayer, bayer_coord + g_off).x,
           texture2D(bayer, bayer_coord + b_off).x) * div;

  vec4 raw = texture2D(framebuffer, tex_coord);
  vec4 adjusted = raw + vec4(bayer_val, 0.0);
  gl_FragColor = adjusted - mod(adjusted, div);
}
