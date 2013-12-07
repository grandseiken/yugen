// Currently disabled, since I like how it looks without gamma-correction,
// and don't want to turn correction on until it looks as nice.
// In particular, ignoring gamma means colours don't shade exactly linearly,
// which kind of looks good.
const bool use_gamma_correction = false;
const float gamma_value = 2.2;
const float gamma_div = 1.0 / gamma_value;

vec4 gamma_write(vec4 v)
{
  if (!use_gamma_correction) {
    return v;
  }
  return vec4(pow(v.r, gamma_value),
              pow(v.g, gamma_value),
              pow(v.b, gamma_value),
              v.a);
}

vec4 gamma_read(vec4 v)
{
  if (!use_gamma_correction) {
    return v;
  }
  return vec4(pow(v.r, gamma_div),
              pow(v.g, gamma_div),
              pow(v.b, gamma_div),
              v.a);
}
