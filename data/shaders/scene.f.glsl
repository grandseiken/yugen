uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
varying vec2 tex_coord;

#include "light_util.glsl"

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec3 light = cel_shade(vec3(texture2D(lightbuffer, tex_coord)));
  gl_FragColor = colour * vec4(light.r, light.g, light.b, 1.0);
}
