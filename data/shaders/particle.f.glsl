flat varying vec4 colour_coord;
flat varying float depth_coord;

void main()
{
  if (colour_coord.a == 0) {
    discard;
  }

  gl_FragColor = colour_coord;
  gl_FragDepth = depth_coord;
}
