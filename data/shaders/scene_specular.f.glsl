uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
varying vec2 tex_coord;

#include "light_util.glsl"

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  float light = cel_shade(vec3(texture2D(lightbuffer, tex_coord)).r, true);
  colour.a *= light;
  gl_FragColor = colour;
}
