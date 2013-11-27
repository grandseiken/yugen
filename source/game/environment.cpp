#include "environment.h"

#include "../perlin.h"
#include "../render/gl_util.h"
#include "../render/util.h"

#include <algorithm>

Particle::Particle(
    y::int32 tag, y::int32 frames, y::int32 size,
    y::world depth, y::world layering_value,
    const y::wvec2& p, const y::wvec2& dp, const y::wvec2 d2p,
    const y::fvec4& colour, const y::fvec4& dcolour, const y::fvec4& d2colour)
  : tag(tag)
  , frames(frames)
  , size(size)
  , depth(depth)
  , layering_value(layering_value)
  , p(p)
  , dp(dp)
  , d2p(d2p)
  , colour(colour)
  , dcolour(colour)
  , d2colour(d2colour)
{
}

bool Particle::update()
{
  p += dp;
  dp += d2p;
  colour += dcolour;
  dcolour += d2colour;

  return --frames >= 0;
}

Environment::Environment(GlUtil& gl, bool fake)
  : _pixels(gl.make_unique_buffer<float, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour(gl.make_unique_buffer<float, 4>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _depth(gl.make_unique_buffer<float, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _particle_program(gl.make_unique_program({
        "/shaders/particle.v.glsl",
        "/shaders/particle.f.glsl"}))
  , _fog_program(gl.make_unique_program({
        "/shaders/env/fog.v.glsl",
        "/shaders/env/fog.f.glsl"}))
  , _reflect_program(gl.make_unique_program({
        "/shaders/env/reflect.v.glsl",
        "/shaders/env/reflect.f.glsl"}))
  , _reflect_normal_program(gl.make_unique_program({
        "/shaders/env/reflect_normal.v.glsl",
        "/shaders/env/reflect_normal.f.glsl"}))
{
  if (fake) {
    return;
  }

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

void Environment::add_particle(const Particle& particle)
{
  _particles.push_back(particle);
}

void Environment::destroy_particles(y::int32 tag)
{
  _particles.erase(std::remove_if(
        _particles.begin(), _particles.end(),
        [tag](Particle& p) {return p.tag == tag;}), _particles.end());
}

void Environment::modify_particles(
    y::int32 tag,
    const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add)
{
  // If iterating over the entire particle list every time we want to tweak them
  // is too much, could map tags to separate lists.
  for (Particle& p : _particles) {
    if (p.tag != tag) {
      continue;
    }
    p.p += p_add;
    p.dp += dp_add;
    p.d2p += d2p_add;
  }
}

void Environment::update_particles()
{
  _particles.erase(std::remove_if(
        _particles.begin(), _particles.end(),
        [](Particle& p) {return !p.update();}), _particles.end());
}

void Environment::render_particles(RenderUtil& util,
                                   const y::wvec2& camera) const
{
  GlUtil& gl = util.get_gl();

  gl.enable_depth(true);
  gl.enable_blend(true);

  y::size length = _particles.size();
  for (y::size i = 0; i < length; ++i) {
    const Particle& p = _particles[i];

    // Divisor buffers would be great, again.
    // TODO: write pixel buffer.
    y::write_vector(_colour.data, 16 * i, {
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa]});
    y::write_vector<float, y::vector<double>>(_depth.data, 4 * i, {
        p.depth, p.depth, p.depth, p.depth});
  }

  _particle_program->bind();
  _particle_program->bind_attribute("pixels", _pixels.reupload());
  _particle_program->bind_attribute("colour", _colour.reupload());
  _particle_program->bind_attribute("depth", _depth.reupload());

  util.bind_pixel_uniforms(*_particle_program);
  util.quad_element(length).buffer->draw_elements(GL_TRIANGLES, 6 * length);
}

void Environment::render_fog_colour(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const fog_params& params) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);
  auto pixel_buffer = make_rect_buffer(util, origin, region);

  _fog_program->bind();
  _fog_program->bind_attribute("pixels", *pixel_buffer);
  _fog_program->bind_uniform("perlin_size", _f3d_128->get_size());
  _fog_program->bind_uniform("perlin", *_f3d_128);

  _fog_program->bind_uniform(
      "tex_offset", -y::fvec2(params.tex_offset + origin));
  _fog_program->bind_uniform("frame", float(params.frame));
  _fog_program->bind_uniform("colour", params.colour);
  _fog_program->bind_uniform("fog_min", float(params.fog_min));
  _fog_program->bind_uniform("fog_max", float(params.fog_max));

  util.bind_pixel_uniforms(*_fog_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Environment::render_fog_normal(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const fog_params& params) const
{
  util.render_fill(y::fvec2(origin - region / 2), y::fvec2(region),
                   y::fvec4{.5f, .5f, float(params.layering_value),
                            params.normal_only ? params.colour[aa] : 1.f});
}

void Environment::render_reflect_colour(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const reflect_params& params, const GlFramebuffer& source) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(false);
  auto pixel_buffer = make_rect_buffer(util, origin, region);

  _reflect_program->bind();
  _reflect_program->bind_attribute("pixels", *pixel_buffer);
  _reflect_program->bind_uniform("perlin_size", _fv23d_64->get_size());
  _reflect_program->bind_uniform("perlin", *_fv23d_64);
  _reflect_program->bind_uniform("source", source);

  _reflect_program->bind_uniform("tex_offset",
                                 -y::fvec2(params.tex_offset + origin));
  _reflect_program->bind_uniform("frame", float(params.frame));
  _reflect_program->bind_uniform("colour", params.colour);
  _reflect_program->bind_uniform("reflect_mix", float(params.reflect_mix));
  _reflect_program->bind_uniform("light_passthrough",
                                 float(params.light_passthrough));

  _reflect_program->bind_uniform("normal_scaling_reflect",
                                 float(params.normal_scaling_reflect));
  _reflect_program->bind_uniform("normal_scaling_refract",
                                 float(params.normal_scaling_refract));

  _reflect_program->bind_uniform("reflect_fade_start",
                                 float(params.reflect_fade_start));
  _reflect_program->bind_uniform("reflect_fade_end",
                                 float(params.reflect_fade_end));

  _reflect_program->bind_uniform("flip_x", params.flip_x);
  _reflect_program->bind_uniform("flip_y", params.flip_y);
  _reflect_program->bind_uniform("flip_axes", y::fvec2(params.flip_axes));

  _reflect_program->bind_uniform("wave_height", float(params.wave_height));
  _reflect_program->bind_uniform("wave_scale", float(params.wave_scale));

  util.bind_pixel_uniforms(*_reflect_program);
  util.quad_element().draw_elements(GL_TRIANGLE_STRIP, 4);
}

void Environment::render_reflect_normal(
    RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
    const reflect_params& params) const
{
  util.get_gl().enable_depth(false);
  util.get_gl().enable_blend(params.normal_only);
  auto pixel_buffer = make_rect_buffer(util, origin, region);

  _reflect_normal_program->bind();
  _reflect_normal_program->bind_attribute("pixels", *pixel_buffer);
  _reflect_normal_program->bind_uniform("perlin_size", _fv23d_64->get_size());
  _reflect_normal_program->bind_uniform("perlin", *_fv23d_64);

  _reflect_normal_program->bind_uniform("layer", float(params.layering_value));
  _reflect_normal_program->bind_uniform("tex_offset",
                                        -y::fvec2(params.tex_offset + origin));
  _reflect_normal_program->bind_uniform("frame", float(params.frame));
  _reflect_normal_program->bind_uniform("colour", params.colour);
  _reflect_normal_program->bind_uniform("normal_scaling",
                                        float(params.normal_scaling));

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
