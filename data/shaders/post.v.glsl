attribute vec2 position;
varying vec2 tex_coord;

void main()
{
  tex_coord = position * vec2(0.5) + vec2(0.5);
  gl_Position = vec4(position, 0.0, 1.0);
}
