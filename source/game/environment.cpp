#include "environment.h"

#include "collision.h"
#include "world.h"
#include "../perlin.h"
#include "../render/gl_util.h"
#include "../render/util.h"

Particle::Particle(
    y::int32 tag, y::int32 frames, y::world bounce_coefficient,
    y::world depth, y::world layering_value,
    y::world size, y::world dsize, y::world d2size,
    const y::wvec2& p, const y::wvec2& dp, const y::wvec2 d2p,
    const y::fvec4& colour, const y::fvec4& dcolour, const y::fvec4& d2colour)
  : tag(tag)
  , frames(frames)
  , bounce_coefficient(bounce_coefficient)
  , depth(depth)
  , layering_value(layering_value)
  , size(size)
  , dsize(dsize)
  , d2size(d2size)
  , p(p)
  , dp(dp)
  , d2p(d2p)
  , colour(colour)
  , dcolour(dcolour)
  , d2colour(d2colour)
{
}

bool Particle::update(const WorldWindow& world)
{
  if (bounce_coefficient < 0.) {
    p += dp;
  }
  else {
    bool collision = false;
    y::world min_ratio = 1;
    y::wvec2 nearest_normal;

    y::wvec2 min = p - y::wvec2{size, size};
    y::wvec2 max = p + y::wvec2{size, size};
    auto it = world.get_geometry().search(
        y::min(min, min + dp), y::max(max, max + dp));

    for (; it && min_ratio > 0.; ++it) {
      // Set Collision::collider_move for details.
      y::wvec2 start = y::wvec2(it->start);
      y::wvec2 end = y::wvec2(it->end);
      y::wvec2 vec = end - start;
      y::wvec2 normal{vec[yy], -vec[xx]};

      if (normal.dot(-dp) <= 0) {
        continue;
      }

      y::world ratio = get_projection(start, end, p, dp, true);
      if (min_ratio > ratio) {
        collision = true;
        min_ratio = ratio;
        nearest_normal = normal;
      }
    }

    if (collision) {
      p += y::max(0., min_ratio) * dp;

      // Reflect the velocity vector in the nearest geometry and apply the bounce
      // coefficient.
      nearest_normal.normalise();
      dp = 2 * dp.dot(nearest_normal) * nearest_normal - dp;
      dp *= -bounce_coefficient;
    }
    else {
      p += dp;
    }
  }

  dp += d2p;
  colour += dcolour;
  dcolour += d2colour;
  size += dsize;
  dsize += d2size;

  return --frames >= 0;
}

void Particle::modify(
    const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add)
{
  p += p_add;
  dp += dp_add;
  d2p += d2p_add;
}

Environment::Environment(GlUtil& gl, const WorldWindow& world, bool fake)
  : _world(world)
  , _pixels(gl.make_unique_buffer<float, 2>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _colour(gl.make_unique_buffer<float, 4>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _depth(gl.make_unique_buffer<float, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _layering(gl.make_unique_buffer<float, 1>(
        GL_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _element(gl.make_unique_buffer<GLushort, 1>(
        GL_ELEMENT_ARRAY_BUFFER, GL_STREAM_DRAW))
  , _particle_program(gl.make_unique_program({
        "/shaders/particle.v.glsl",
        "/shaders/particle.f.glsl"}))
  , _particle_normal_program(gl.make_unique_program({
        "/shaders/particle_normal.v.glsl",
        "/shaders/particle_normal.f.glsl"}))
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
  _particles.erase(y::remove_if(
        _particles.begin(), _particles.end(),
        [tag](Particle& p) {return p.tag == tag;}), _particles.end());
}

void Environment::destroy_particles()
{
  _particles.clear();
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
    p.modify(p_add, dp_add, d2p_add);
  }
}

void Environment::modify_particles(
    const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add)
{
  for (Particle& p : _particles) {
    p.modify(p_add, dp_add, d2p_add);
  }
}

void Environment::update_particles()
{
  _particles.erase(
      y::remove_if(
          _particles.begin(), _particles.end(),
          [this](Particle& p) {return !p.update(this->_world);}),
      _particles.end());
}

void Environment::render_particles(RenderUtil& util) const
{
  GlUtil& gl = util.get_gl();

  gl.enable_depth(true);
  gl.enable_blend(true);

  y::size length = _particles.size();
  for (y::size i = 0; i < length; ++i) {
    const Particle& p = _particles[i];
    y::wvec2 pos = p.p;

    // Divisor buffers would be great, again.
    y::write_vector<float, y::vector<y::world>>(_pixels.data, 8 * i, {
        pos[xx] - p.size / 2., pos[yy] - p.size / 2.,
        pos[xx] + p.size / 2., pos[yy] - p.size / 2.,
        pos[xx] - p.size / 2., pos[yy] + p.size / 2.,
        pos[xx] + p.size / 2., pos[yy] + p.size / 2.});
    y::write_vector(_colour.data, 16 * i, {
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa],
        p.colour[rr], p.colour[gg], p.colour[bb], p.colour[aa]});
    y::write_vector<float, y::vector<y::world>>(_depth.data, 4 * i, {
        p.depth, p.depth, p.depth, p.depth});
  }

  _particle_program->bind();
  _particle_program->bind_attribute("pixels", _pixels.reupload());
  _particle_program->bind_attribute("colour", _colour.reupload());
  _particle_program->bind_attribute("depth", _depth.reupload());

  util.bind_pixel_uniforms(*_particle_program);
  util.quad_element(length).buffer->draw_elements(GL_TRIANGLES, 6 * length);
}

void Environment::render_particles_normal(RenderUtil& util) const
{
  GlUtil& gl = util.get_gl();

  gl.enable_depth(true);
  gl.enable_blend(false);

  y::size length = _particles.size();
  for (y::size i = 0; i < length; ++i) {
    const Particle& p = _particles[i];
    y::wvec2 pos = p.p;

    // Divisor buffers would be great, again.
    y::write_vector<float, y::vector<y::world>>(_pixels.data, 8 * i, {
        pos[xx] - p.size / 2., pos[yy] - p.size / 2.,
        pos[xx] + p.size / 2., pos[yy] - p.size / 2.,
        pos[xx] - p.size / 2., pos[yy] + p.size / 2.,
        pos[xx] + p.size / 2., pos[yy] + p.size / 2.});
    y::write_vector<float, y::vector<y::world>>(_depth.data, 4 * i, {
        p.depth, p.depth, p.depth, p.depth});
    y::write_vector<float, y::vector<y::world>>(_layering.data, 4 * i, {
        p.layering_value, p.layering_value,
        p.layering_value, p.layering_value});
  }

  _particle_normal_program->bind();
  _particle_normal_program->bind_attribute("pixels", _pixels.reupload());
  _particle_normal_program->bind_attribute("depth", _depth.reupload());
  _particle_normal_program->bind_attribute("layering_value",
                                           _layering.reupload());

  util.bind_pixel_uniforms(*_particle_normal_program);
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
