uniform sampler2D sprite;
uniform ivec2 frame_size;
uniform ivec2 frame_count;
uniform sampler2D lightbuffer;
uniform bool use_lightbuffer;

varying vec2 tex_coord;
varying vec2 pos_coord;
varying vec2 frame_coord;
varying vec4 colour_coord;
varying float depth_coord;

const float ambient_light = 0.1;

void main()
{
  vec2 coord = mod(frame_coord, frame_count) +
      mod(tex_coord, vec2(frame_size)) / frame_size;

  // Snap to texture pixels. This is necessary to avoid occasionally taking
  // pixels from the next frame over at the very edge when rotating.
  coord = coord - mod(coord, 1.0 / vec2(frame_size));

  vec4 colour = texture2D(sprite, coord / frame_count) * colour_coord;
  if (use_lightbuffer) {
    float light = max(ambient_light, texture2D(lightbuffer, pos_coord).r);
    colour = vec4(light * colour.r, light * colour.g, light * colour.b,
                  colour.a);
  }

  // Don't write the depth buffer.
  if (colour.a == 0) {
    discard;
  }
  gl_FragColor = colour;
  gl_FragDepth = depth_coord;
}
