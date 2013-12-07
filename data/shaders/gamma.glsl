// Disabled. See comments in gl_util.cpp.
const bool use_gamma_correction = false;
const float gamma_value = 2.2;
const float gamma_div = 1.0 / gamma_value;

vec4 gamma_correction(vec4 v)
{
  if (!use_gamma_correction) {
    return v;
  }
  return vec4(pow(v.r, gamma_div),
              pow(v.g, gamma_div),
              pow(v.b, gamma_div),
              v.a);
}

