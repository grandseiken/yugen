#include "environment.h"
#include "perlin.h"
#include "render_util.h"

Environment::Environment(GlUtil& gl)
  : _fog_program(gl.make_program({
      "/shaders/fog.v.glsl",
      "/shaders/fog.f.glsl"}))
{
  typedef Perlin<float> fperlin;
  typedef Perlin<y::fvec2> fv2perlin;

  fperlin fp;

  // 2D perlin field with 32x32 8x8 patches, down to 2x2 128x128 patches.
  fperlin::field f2d_256;
  fp.generate_perlin<2>(f2d_256, 32, 8, 5);
  _f2d_256 = gl.make_texture({256, 256}, GL_FLOAT, GL_R8, GL_RED,
                             f2d_256.data(), true);

  // TODO: test.
  fv2perlin fv2p;
  (void)fv2p;
}

void Environment::render(
    RenderUtil& util, const y::wvec2& camera_min, const y::wvec2& camera_max,
    const GlFramebuffer& colourbuffer, const GlFramebuffer& normalbuffer) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);

  y::vector<float> pixel_data{
      float(camera_min[xx]), float(camera_min[yy]),
      float(camera_max[xx]), float(camera_min[yy]),
      float(camera_min[xx]), float(camera_max[yy]),
      float(camera_max[xx]), float(camera_max[yy])}; 
  GlBuffer<float, 2> pixel_buffer(util.get_gl().make_buffer<float, 2>(
      GL_ARRAY_BUFFER, GL_STREAM_DRAW, pixel_data));

  colourbuffer.bind(true, true);
  _fog_program.bind();
  _fog_program.bind_attribute("pixels", pixel_buffer);
  _fog_program.bind_uniform("perlin_size", _f2d_256.get_size());
  _fog_program.bind_uniform("perlin", _f2d_256);
  _fog_program.bind_uniform("colour", y::fvec4{.5f, .5f, .5f, .4f});
  util.bind_pixel_uniforms(_fog_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);

  util.get_gl().delete_buffer(pixel_buffer);

  normalbuffer.bind(true, false);
  util.clear({.5f, .5f, 0.f, 1.f});
}
