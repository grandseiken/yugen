uniform vec4 colour;
uniform ivec3 perlin_size;
uniform sampler3D perlin;
uniform int frame;
varying vec2 tex_coord;

const float fog_min = 0.4;
const float fog_max = 0.7;
const float fog_scale = 1.0 / (fog_max - fog_min);

void main()
{
  // Offset z by some non-integral division of the x and y in order to cut an
  // odd slice through the 3D texture and avoid obvious repetition even with a
  // small perlin texture.
  float z = frame / (8.0 * perlin_size.z);
  z += tex_coord.x / (2.0 + 1.0 / 3.0) + tex_coord.y / (3 + 2.0 / 3.0);

  float p = texture3D(perlin, vec3(tex_coord.x, tex_coord.y, z)).x;
  if (p < fog_min) {
    discard;
  }
  vec4 c = colour;
  c.a *= fog_scale * (p - fog_min);
  gl_FragColor = c;
}
