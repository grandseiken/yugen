#version 120

uniform float fade_factor;
uniform sampler2D textures[2];
varying vec2 tex_coord;

void main()
{
  gl_FragColor = mix(
      texture2D(textures[0], tex_coord), texture2D(textures[1], tex_coord),
      fade_factor);
}
