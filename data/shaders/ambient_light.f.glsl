uniform sampler2D colourbuffer;
varying vec2 tex_coord;

const float ambient_light = 0.1;

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  gl_FragColor = vec4(colour.r, colour.g, colour.b, ambient_light);
}
