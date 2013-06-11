uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
varying vec2 tex_coord;

const float ambient_light = 0.05;

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec4 light = texture2D(lightbuffer, tex_coord);
  light = min(light + vec4(ambient_light, ambient_light, ambient_light, 0.0),
              vec4(1.0, 1.0, 1.0, 1.0));
  gl_FragColor = colour * light;
}
