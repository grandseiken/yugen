attribute vec2 position;
uniform ivec2 native_res;
uniform ivec2 native_overflow_res;
uniform float rotation;
varying vec2 tex_coord;

void main()
{
  mat2 rot = mat2(cos(rotation), sin(rotation),
                  -sin(rotation), cos(rotation));

  vec2 crop_position = position * vec2(native_res) / vec2(native_overflow_res);
  tex_coord = 0.5 + 0.5 * rot * crop_position;
  gl_Position = vec4(position, 0.0, 1.0);
}
