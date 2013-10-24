attribute vec2 position;
uniform vec2 dither_off;
uniform float dither_rot;
uniform ivec2 native_res;
noperspective varying vec2 tex_coord;
noperspective varying vec2 dither_coord;

// "Correct" dithering makes more sense mathematically (i.e. dithering in world
// coordinates), but due to pixelated rotation accuracies produces unfortunate
// Moire patterns any time the camera is not axis-aligned.
// When turned off, we use a hack to keep dithering axis-aligned in camera space
// (i.e. rotation-independent) that is nicer in general; the downside is the
// dithering shifts independently of the world as the camera rotates.
bool correct_dithering = false;

void main()
{
  tex_coord = 0.5 + 0.5 * position;
  gl_Position = vec4(position, 0.0, 1.0);

  vec2 off = dither_off - mod(dither_off, 1.0);
  if (correct_dithering) {
    mat2 rot = mat2(cos(dither_rot), -sin(dither_rot),
                    sin(dither_rot), cos(dither_rot));
    dither_coord = off +
        rot * (vec2(tex_coord.x - 0.5, 0.5 - tex_coord.y) * vec2(native_res));
  }
  else {
    mat2 rot = mat2(cos(dither_rot), sin(dither_rot),
                    -sin(dither_rot), cos(dither_rot));
    dither_coord = rot * off +
        vec2(tex_coord.x, -tex_coord.y) * vec2(native_res);
  }
}
