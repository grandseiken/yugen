uniform vec4 colour;
uniform sampler2D perlin;
varying vec2 tex_coord;

const float fog_min = 0.4;
const float fog_max = 0.7;
const float fog_scale = 1.0 / (fog_max - fog_min);

void main()
{
  float p = texture2D(perlin, tex_coord).x;
  if (p < fog_min) {
    discard;
  }
  vec4 c = colour;
  c.a *= fog_scale * (p - fog_min);
  gl_FragColor = c;
}
