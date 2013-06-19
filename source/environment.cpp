#include "environment.h"
#include "perlin.h"
#include "gl_util.h"
#include "render_util.h"

Environment::Environment(GlUtil& gl)
  : _fog_program(gl.make_unique_program({
      "/shaders/fog.v.glsl",
      "/shaders/fog.f.glsl"}))
{
  typedef Perlin<float> fperlin;
  typedef Perlin<y::fvec2> fv2perlin;

  fperlin fp(123648);

  // 2D perlin field with 32x32 8x8 patches, down to 2x2 128x128 patches.
  fperlin::field f2d_256;
  fp.generate_perlin<2>(f2d_256, 32, 8, 5);
  _f2d_256.swap(gl.make_unique_texture(
      y::ivec2{256, 256}, GL_R8, GL_RED, f2d_256.data(), true));

  // 3D perlin field of size 128x128x128.
  fperlin::field f3d_128;
  LinearWeights<float> weights(1.f, 1.f);
  fp.generate_perlin<3>(f3d_128, 64, 2, 6, weights);
  _f3d_128.swap(gl.make_unique_texture<float, 3>(
      y::ivec3{128, 128, 128}, GL_R8, GL_RED, f3d_128.data(), true));

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
  auto pixel_buffer =
      util.get_gl().make_unique_buffer<float, 2>(
          GL_ARRAY_BUFFER, GL_STREAM_DRAW, pixel_data);

  colourbuffer.bind(true, true);
  _fog_program->bind();
  _fog_program->bind_attribute("pixels", *pixel_buffer);
  _fog_program->bind_uniform("perlin_size", _f3d_128->get_size());
  _fog_program->bind_uniform("perlin", *_f3d_128);
  _fog_program->bind_uniform("colour", y::fvec4{.5f, .5f, .5f, .25f});
  _fog_program->bind_uniform("frame", _frame++);
  util.bind_pixel_uniforms(*_fog_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);

  normalbuffer.bind(true, false);
  util.clear({.5f, .5f, 0.f, 1.f});
}
