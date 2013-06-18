uniform vec4 colour;
uniform ivec3 perlin_size;
uniform sampler3D perlin;
uniform int frame;
varying vec2 tex_coord;

const float fog_min = 0.4;
const float fog_max = 0.8;
const float fog_scale = 1.0 / (fog_max - fog_min);

void main()
{
  float p = texture3D(perlin, vec3(tex_coord.x, tex_coord.y,
                                   frame / (16.0 * perlin_size.z))).x;
  if (p < fog_min) {
    discard;
  }
  vec4 c = colour;
  c.a *= fog_scale * (p - fog_min);
  gl_FragColor = c;
}
