uniform sampler2D sprite;
uniform ivec2 frame_size;
uniform ivec2 frame_count;
varying vec2 tex_coord;
varying vec2 frame_coord;
varying vec4 colour_coord;
varying float depth_coord;

void main()
{
  vec2 coord = mod(frame_coord, frame_count) +
      mod(tex_coord, vec2(frame_size)) / frame_size;
  vec4 color = texture2D(sprite, coord / frame_count) * colour_coord;
  // Don't write the depth buffer.
  if (color.a == 0) {
    discard;
  }
  gl_FragColor = color;
  gl_FragDepth = depth_coord;
}
