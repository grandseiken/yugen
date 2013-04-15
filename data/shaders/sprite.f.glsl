uniform sampler2D sprite;
uniform ivec2 frame_size;
uniform ivec2 frame_count;
varying vec2 tex_coord;
varying vec2 frame_coord;

void main()
{
  vec2 coord = mod(frame_coord, frame_count) +
      mod(tex_coord, vec2(frame_size)) / frame_size;
  gl_FragColor = texture2D(sprite, coord / frame_count);
}
