#version 120

uniform sampler2D framebuffer;
varying vec2 tex_coord;

void main()
{
  gl_FragColor = texture2D(framebuffer, tex_coord);
}
