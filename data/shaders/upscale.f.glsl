uniform sampler2D framebuffer;
uniform ivec2 screen_res;
uniform ivec2 native_res;
uniform bool integral_scale_lock;
uniform bool use_epx;
uniform bool use_fra;
varying vec2 tex_coord;

bool eq(vec4 a, vec4 b)
{
  return a.r == b.r && a.g == b.g && a.b == b.b;
}

// Fra scale. Does the two-step process of inflating to an arbitrarily high
// resolution with no interpolation, followed by scaling back down to the
// desired size with linear interpolation. AKA BLur-O-Vision.
// I don't know if this idea has been around for longer but Simon "Fra" Howard
// of Chocolate Doom fame told me about his idea for it.
vec4 fra_scale()
{
  vec2 screen_size = 1.0 / screen_res;
  vec2 native_size = 1.0 / native_res;

  // Lengths by which the current scaled pixel overlaps the previous and
  // next source pixels.
  vec2 prev_weight =
      native_size - mod(tex_coord, native_size);
  vec2 next_weight =
      mod(tex_coord + screen_size, native_size);

  // Look up the four colours to interpolate.
  vec4 pp = texture2D(framebuffer, tex_coord);
  vec4 np = texture2D(framebuffer, tex_coord + vec2(screen_size.x, 0.0));
  vec4 pn = texture2D(framebuffer, tex_coord + vec2(0.0, screen_size.y));
  vec4 nn = texture2D(framebuffer, tex_coord + vec2(screen_size.x,
                                                    screen_size.y));

  // Weight by the overlap areas.
  float pp_weight = prev_weight.x * prev_weight.y;
  float np_weight = next_weight.x * prev_weight.y;
  float pn_weight = prev_weight.x * next_weight.y;
  float nn_weight = next_weight.x * next_weight.y;

  vec4 mix = pp * pp_weight + np * np_weight + pn * pn_weight + nn * nn_weight;
  float d = pp_weight + np_weight + pn_weight + nn_weight;
  return mix / d;
}

// 2xEPX. Doesn't work.
// TODO: make work. Do 3xEPX, 4xEPX, other algorithms?
vec4 epx_scale()
{
  vec2 screen_size = 1.0 / vec2(screen_res);
  vec2 native_size = 1.0 / vec2(native_res);

  vec2 t = mod(tex_coord / screen_size, 2.0);
  bvec2 control = lessThan(t, vec2(1.0, 1.0));

  vec4 u = texture2D(framebuffer, tex_coord - vec2(0.0, native_size.y));
  vec4 d = texture2D(framebuffer, tex_coord + vec2(0.0, native_size.y));
  vec4 l = texture2D(framebuffer, tex_coord - vec2(native_size.x, 0.0));
  vec4 r = texture2D(framebuffer, tex_coord + vec2(native_size.x, 0.0));

  if (control.x) {
    if (!control.y) {
      if (eq(l, u) && !eq(l, d) && !eq(u, r)) {
        return u;
      }
    }
    else {
      if (eq(d, l) && !eq(d, r) && !eq(l, u)) {
        return l;
      }
    }
  }
  else {
    if (!control.y) {
      if (eq(u, r) && !eq(u, l) && !eq(r, d)) {
        return r;
      }
    }
    else {
      if (eq(r, d) && !eq(r, u) && !eq(d, l)) {
        return d;
      }
    }
  }

  return texture2D(framebuffer, tex_coord);
}

void main()
{
  bool integral = integral_scale_lock ||
                  mod(screen_res / native_res, 1.0) == vec2(0.0);
  if (use_fra && !integral) {
    gl_FragColor = fra_scale();
  }
  else if (use_epx && integral) {
    gl_FragColor = epx_scale();
  }
  else {
    gl_FragColor = texture2D(framebuffer, tex_coord);
  }
}
