uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
varying vec2 tex_coord;

const float ambient_light = 0.05;
const float cel_shade = 1.0 / 4;

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec4 light = texture2D(lightbuffer, tex_coord);

  light = min(light + vec4(ambient_light, ambient_light, ambient_light, 0.0),
              vec4(1.0, 1.0, 1.0, 1.0));

  // Experimental mixing in of cel-shading.
  vec4 cel = vec4(cel_shade, cel_shade, cel_shade, 1.0);
  vec4 c_light = light + cel / 2;
  c_light = c_light - mod(c_light, cel);
  light = (4 * light + c_light) / 5;

  gl_FragColor = colour * light;
}
