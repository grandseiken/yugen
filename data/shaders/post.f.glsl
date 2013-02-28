#version 120

uniform sampler2D framebuffer;
uniform ivec2 native_width;
uniform ivec2 screen_width;
varying vec2 tex_coord;

void main()
{
  gl_FragColor = texture2D(framebuffer, tex_coord);
}
