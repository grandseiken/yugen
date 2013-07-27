float perlin_z(vec2 tex_coord, float z)
{
  // Offset z by some non-integral division of the x and y in order to cut an
  // odd slice through the 3D texture and avoid obvious repetition even with a
  // small perlin texture.
  return z + tex_coord.x / (2.0 + 1.0 / 3.0) + tex_coord.y / (3 + 2.0 / 3.0);
}

vec4 perlin_lookup(sampler3D perlin, vec2 tex_coord, float z)
{
  return texture3D(perlin, vec3(tex_coord.xy, perlin_z(tex_coord, z)));
}
