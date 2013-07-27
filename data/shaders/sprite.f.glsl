uniform sampler2D sprite;
uniform ivec2 frame_size;
uniform ivec2 frame_count;
uniform bool normal;

noperspective varying vec2 tex_coord;
flat varying vec2 frame_coord;
flat varying vec4 colour_coord;
flat varying float depth_coord;

void main()
{
  vec2 coord = mod(frame_coord, frame_count) +
      mod(tex_coord, vec2(frame_size)) / frame_size;

  // Snap to texture pixels. This is necessary to avoid occasionally taking
  // pixels from the next frame over at the very edge when rotating.
  coord = coord - mod(coord, 1.0 / vec2(frame_size));

  // Base colour.
  vec4 colour = texture2D(sprite, coord / frame_count);
  if (normal) {
    colour.b += depth_coord;
  }
  else {
    colour *= colour_coord;
  }

  // Don't write the depth buffer.
  if (colour.a == 0) {
    discard;
  }

  gl_FragColor = colour;
  gl_FragDepth = depth_coord;
}
