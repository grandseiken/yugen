attribute vec2 position;
varying vec2 tex_coord;

void main()
{
  tex_coord = vec2(0.5) + 0.5 * position;
  gl_Position = vec4(position, 0.0, 1.0);
}
