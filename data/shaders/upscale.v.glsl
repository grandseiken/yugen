attribute vec2 position;
uniform ivec2 native_res;
uniform ivec2 screen_res;
uniform bool integral_scale_lock;
noperspective varying vec2 tex_coord;

void main()
{
  // Correction is needed for floor/division to behave properly. Probably due
  // to awful default drivers on Linux.
  const float correction = 1.0 / 0xffff;
  tex_coord = 0.5 + 0.5 * position;

  vec2 scale_v = vec2(screen_res) / vec2(native_res) + correction;
  float scale = floor(min(scale_v.x, scale_v.y));

  vec2 viewport = 1.0;
  if (scale >= 1.0 && integral_scale_lock) {
    viewport = scale * native_res / vec2(screen_res);
  }
  else {
    float native_aspect = float(native_res.x) / native_res.y;
    float screen_aspect = float(screen_res.x) / screen_res.y;
    if (native_aspect >= screen_aspect) {
      viewport = vec2(1.0, screen_aspect / native_aspect);
    }
    else {
      viewport = vec2(native_aspect / screen_aspect, 1.0);
    }
  }
  gl_Position = vec4(position * viewport, 0.0, 1.0);
}
