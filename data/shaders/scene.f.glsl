uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
noperspective varying vec2 tex_coord;

#include "light_util.glsl"

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec3 light = cel_shade(vec3(texture2D(lightbuffer, tex_coord)), true, false);
  gl_FragColor = colour * vec4(light.rgb, 1.0);
}
