uniform sampler2D colourbuffer;
uniform sampler2D normalbuffer;
uniform vec2 light_direction;
uniform vec4 layer_colour;
varying vec2 tex_coord;

#include "light_util.glsl"

void main()
{
  vec4 colour = texture2D(colourbuffer, tex_coord);
  vec3 normal = tex_to_world_normal(texture2D(normalbuffer, tex_coord));
  vec3 light_normal = sq_coords_to_world_normal(light_direction);
  float light = 0.5 + light_value(light_normal, normal) / 2.0;
  vec3 light_c = cel_shade(vec3(light, light, light), true, false);

  gl_FragColor = colour * layer_colour *
      vec4(light_c.r, light_c.g, light_c.b, 1.0);
}
