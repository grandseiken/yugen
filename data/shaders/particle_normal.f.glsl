flat varying float depth_coord;
flat varying float layering_coord;

void main()
{
  gl_FragColor = vec4(0.5, 0.5, layering_coord, 1.0);
  gl_FragDepth = depth_coord;
}
