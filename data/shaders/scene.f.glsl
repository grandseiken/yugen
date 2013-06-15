uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
varying vec2 tex_coord;

const float ambient_light = 0.05;
const float cel_shade_clamp = 1.0 / 4;
const float cel_shade_mix = 0.0;

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec4 light = texture2D(lightbuffer, tex_coord);

  light = min(light + vec4(ambient_light, ambient_light, ambient_light, 0.0),
              vec4(1.0, 1.0, 1.0, 1.0));

  // Experimental mixing in of cel-shading.
  vec4 cel = vec4(cel_shade_clamp, cel_shade_clamp, cel_shade_clamp, 1.0);
  vec4 c_light = light + 0.5 * cel;
  c_light = c_light - mod(c_light, cel);
  light = cel_shade_mix *  c_light + (1 - cel_shade_mix) * light;

  gl_FragColor = colour * light;
}
