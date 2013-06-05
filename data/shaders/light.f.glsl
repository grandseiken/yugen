varying vec2 pixels_coord;
varying vec2 origin_coord;
varying float range_coord;
varying float intensity_coord;

void main()
{
  vec2 dist_v = pixels_coord - origin_coord;
  float dist_sq = dist_v.x * dist_v.x + dist_v.y * dist_v.y;
  float range_sq = range_coord * range_coord;

  if (dist_sq > range_sq) {
    discard;
  }
  gl_FragColor = vec4(1.0, 1.0, 1.0,
                      intensity_coord * (1.0 - dist_sq / range_sq));
}
