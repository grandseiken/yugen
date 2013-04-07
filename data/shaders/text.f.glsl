#version 120

uniform sampler2D font;
uniform ivec2 font_size;
uniform int string[1024];
uniform vec4 colour;
varying vec2 tex_coord;

void main()
{
  int char = string[int(tex_coord.x) / font_size.x];
  if (char < 32) {
    discard;
  }
  vec2 char_v = vec2(mod(char, 32), char / 32);

  vec2 coord = char_v + mod(tex_coord, font_size) / font_size;
  vec4 lookup = texture2D(font, coord / vec2(32, 4));
  gl_FragColor = vec4(
      colour.r, colour.g, colour.b,
      (1.0 - (lookup.r + lookup.g + lookup.b) / 3.0) * colour.a);
}
