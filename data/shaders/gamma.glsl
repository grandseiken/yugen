// Textures are stored in sRGB, which *appears* linear (i.e. 0.5 grey appears to
// be half-way in between black and white). But in fact the human eye is
// nonlinear, and these values are really "too dark". We need to convert
// textures (or any input colours which are intended to appear linearly) to
// a true linear space (which *appears* too bright to the human eye) before
// performing lighting calculations, and then reverse this again before display.
//
// On the other hand, blending *colours* should really be done in the nonlinear
// output space, since for examplei, a blend value of 0.5 really means we want
// an output half-way between the two colours we would otherwise see.
// TODO: the gamma correction, right now, is happening in the wrong place (input
// of textures and final output only) w.r.t. paragraph 2 and the "other input
// colours" section of paragraph 1.
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

vec3 gamma_write(vec3 v)
{
  if (!use_gamma_correction) {
    return v;
  }
  return vec3(pow(v.r, gamma_value),
              pow(v.g, gamma_value),
              pow(v.b, gamma_value));
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

vec3 gamma_read(vec3 v)
{
  if (!use_gamma_correction) {
    return v;
  }
  return vec3(pow(v.r, gamma_div),
              pow(v.g, gamma_div),
              pow(v.b, gamma_div));
}
