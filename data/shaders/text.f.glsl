uniform sampler2D font;
uniform vec2 font_size;
uniform vec4 colour;
noperspective varying vec2 tex_coord;
flat varying float character_coord;

const int ascii = 128;

void main()
{
  vec2 char_v = vec2(mod(character_coord, ascii), int(character_coord) / ascii);

  vec2 coord = char_v + mod(tex_coord, font_size) / font_size;
  vec4 lookup = texture2D(font, coord / vec2(ascii, 1.0));
  gl_FragColor = vec4(
      colour.rgb, colour.a * lookup.r);
}
