uniform sampler2D framebuffer;
varying vec2 tex_coord;

void main()
{
  // We could try to interpolate but the naive method really ruins the
  // aesthetic. We'd have to use something a bit more clever to simulate
  // manual pixel anti-aliasing without the blurriness.
  gl_FragColor = texture2D(framebuffer, tex_coord);
}
