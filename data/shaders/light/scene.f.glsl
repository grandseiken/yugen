uniform sampler2D colourbuffer;
uniform sampler2D lightbuffer;
noperspective varying vec2 tex_coord;

#include "../gamma.glsl"
#include "util.glsl"

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec3 light = cel_shade(vec3(texture2D(lightbuffer, tex_coord)), true, false);
  // TODO: to do gamma-correct lighting, we need to convert the operands here
  // before the multiplication, and back afterwards. But I'm not exactly sure
  // which way round they need to go.
  gl_FragColor = colour * vec4(light.rgb, 1.0);
}
