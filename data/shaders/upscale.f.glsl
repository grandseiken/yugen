uniform sampler2D framebuffer;
uniform ivec2 screen_res;
uniform ivec2 native_res;
uniform bool use_epx;
varying vec2 tex_coord;

bool eq(vec4 a, vec4 b)
{
  return a.r == b.r && a.g == b.g && a.b == b.b;
}

void main()
{
  if (!use_epx) {
    gl_FragColor = texture2D(framebuffer, tex_coord);
    return;
  }

  vec2 screen_size = 1.0 / vec2(screen_res);
  vec2 native_size = 1.0 / vec2(native_res);

  // 2xEPX. Doesn't work.
  // TODO: make work. Do 3xEPX.
  vec2 t = mod(tex_coord / screen_size, 2.0);
  bvec2 control = lessThan(t, vec2(1.0, 1.0));

  vec4 p = texture2D(framebuffer, tex_coord);
  vec4 u = texture2D(framebuffer, tex_coord - vec2(0.0, native_size.y));
  vec4 d = texture2D(framebuffer, tex_coord + vec2(0.0, native_size.y));
  vec4 l = texture2D(framebuffer, tex_coord - vec2(native_size.x, 0.0));
  vec4 r = texture2D(framebuffer, tex_coord + vec2(native_size.x, 0.0));

  if (control.x) {
    if (!control.y) {
      if (eq(l, u) && !eq(l, d) && !eq(u, r)) {
        p = u;
      }
    }
    else {
      if (eq(d, l) && !eq(d, r) && !eq(l, u)) {
        p = l;
      }
    }
  }
  else {
    if (!control.y) {
      if (eq(u, r) && !eq(u, l) && !eq(r, d)) {
        p = r;
      }
    }
    else {
      if (eq(r, d) && !eq(r, u) && !eq(d, l)) {
        p = d;
      }
    }
  }
  gl_FragColor = p;
}
