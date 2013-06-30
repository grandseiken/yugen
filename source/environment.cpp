#include "environment.h"
#include "perlin.h"
#include "gl_util.h"
#include "render_util.h"

Environment::Environment(GlUtil& gl)
  : _fog_program(gl.make_unique_program({
        "/shaders/env_fog.v.glsl",
        "/shaders/env_fog.f.glsl"}))
  , _reflect_program(gl.make_unique_program({
        "/shaders/env_reflect.v.glsl",
        "/shaders/env_reflect.f.glsl"}))
  , _reflect_normal_program(gl.make_unique_program({
        "/shaders/env_reflect.v.glsl",
        "/shaders/env_reflect_normal.f.glsl"}))
{
  typedef Perlin<float> fperlin;
  typedef Perlin<y::fvec2> fv2perlin;

  fperlin fp(123648);

  // 2D perlin field with 32x32 8x8 patches, down to 2x2 128x128 patches.
  // TODO: actually use this for something or get rid of it.
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

  // 3D perlin field of y::fvec2, size 64x64x64.
  fv2perlin fv2p(57583);
  fv2perlin::field fv23d_64;
  fv2p.generate_perlin<3, LinearWeights<float>, PowerSmoothing<float>>(
      fv23d_64, 64, 1, 4, weights);
  _fv23d_64.swap(gl.make_unique_texture<float, 3>(
      y::ivec3{64, 64, 64}, GL_RG8, GL_RG, &fv23d_64[0][xx], true));
}

void Environment::render_fog_colour(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const y::wvec2& tex_offset, y::world fog_min, y::world fog_max,
    y::world frame, const y::fvec4& colour) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);
  auto pixel_buffer = make_rect_buffer(util, origin, region);
 
  _fog_program->bind();
  _fog_program->bind_attribute("pixels", *pixel_buffer);
  _fog_program->bind_uniform("perlin_size", _f3d_128->get_size());
  _fog_program->bind_uniform("perlin", *_f3d_128);
  _fog_program->bind_uniform("colour", colour);
  _fog_program->bind_uniform("tex_offset", -y::fvec2(tex_offset + origin));
  _fog_program->bind_uniform("fog_min", float(fog_min));
  _fog_program->bind_uniform("fog_max", float(fog_max));
  _fog_program->bind_uniform("frame", float(frame));
  util.bind_pixel_uniforms(*_fog_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Environment::render_fog_normal(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region) const
{
  util.render_fill(y::fvec2(origin - region / 2), y::fvec2(region),
                   y::fvec4{.5f, .5f, 0.f, 1.f});
}

void Environment::render_reflect_colour(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const y::wvec2& tex_offset,
    y::world frame, const y::fvec4& colour,
    float reflect_mix,
    float normal_scaling_reflect, float normal_scaling_refract,
    float reflect_fade_start, float reflect_fade_end,
    bool flip_x, bool flip_y, const y::wvec2& flip_axes,
    const GlFramebuffer& source) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);
  auto pixel_buffer = make_rect_buffer(util, origin, region);
 
  _reflect_program->bind();
  _reflect_program->bind_attribute("pixels", *pixel_buffer);
  _reflect_program->bind_uniform("flip_x", flip_x);
  _reflect_program->bind_uniform("flip_y", flip_y);
  _reflect_program->bind_uniform("flip_axes", y::fvec2(flip_axes));
  _reflect_program->bind_uniform("perlin_size", _fv23d_64->get_size());
  _reflect_program->bind_uniform("perlin", *_fv23d_64);
  _reflect_program->bind_uniform("source", source);
  _reflect_program->bind_uniform("colour", colour);
  _reflect_program->bind_uniform("tex_offset", -y::fvec2(tex_offset + origin));
  _reflect_program->bind_uniform("frame", float(frame));
  _reflect_program->bind_uniform("reflect_mix", reflect_mix);
  _reflect_program->bind_uniform("normal_scaling_reflect",
                                 normal_scaling_reflect);
  _reflect_program->bind_uniform("normal_scaling_refract",
                                 normal_scaling_refract);
  _reflect_program->bind_uniform("reflect_fade_start",
                                 reflect_fade_start);
  _reflect_program->bind_uniform("reflect_fade_end",
                                 reflect_fade_end);
  util.bind_pixel_uniforms(*_reflect_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Environment::render_reflect_normal(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const y::wvec2& tex_offset, y::world frame, float normal_scaling) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);
  auto pixel_buffer = make_rect_buffer(util, origin, region);
 
  _reflect_normal_program->bind();
  _reflect_normal_program->bind_attribute("pixels", *pixel_buffer);
  _reflect_normal_program->bind_uniform("perlin_size", _fv23d_64->get_size());
  _reflect_normal_program->bind_uniform("perlin", *_fv23d_64);
  _reflect_normal_program->bind_uniform("tex_offset", -y::fvec2(tex_offset + origin));
  _reflect_normal_program->bind_uniform("frame", float(frame));
  _reflect_normal_program->bind_uniform("normal_scaling", normal_scaling);
  util.bind_pixel_uniforms(*_reflect_normal_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

GlUnique<GlBuffer<float, 2>> Environment::make_rect_buffer(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region) const
{
  y::vector<float> pixel_data{
      float((origin - region / 2)[xx]), float((origin - region / 2)[yy]),
      float((origin + region / 2)[xx]), float((origin - region / 2)[yy]),
      float((origin - region / 2)[xx]), float((origin + region / 2)[yy]),
      float((origin + region / 2)[xx]), float((origin + region / 2)[yy])};
  return util.get_gl().make_unique_buffer<float, 2>(
      GL_ARRAY_BUFFER, GL_STREAM_DRAW, pixel_data);
}
