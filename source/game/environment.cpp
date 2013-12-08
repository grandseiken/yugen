#include "environment.h"

#include "collision.h"
#include "world.h"
#include "../perlin.h"
#include "../render/gl_util.h"
#include "../render/util.h"
#include "../data/bank.h"
#include "../lua.h"

namespace {

  // Returns the normal of the first wall collided with.
  y::wvec2 mass_collider_update(
      Derivatives<y::wvec2>& mass, const WorldWindow& world)
  {
    mass.d += mass.d2;

    bool collision = false;
    y::world min_ratio = 1;
    y::wvec2 nearest_normal;

    y::wvec2 min = mass.v - y::wvec2{4., 4.};
    y::wvec2 max = mass.v + y::wvec2{4., 4.};
    auto it = world.get_geometry().search(
        y::min(min, min + mass.d), y::max(max, max + mass.d));

    for (; it && min_ratio > 0.; ++it) {
      // Set Collision::collider_move for details.
      y::wvec2 start = y::wvec2(it->start);
      y::wvec2 end = y::wvec2(it->end);
      y::wvec2 vec = end - start;
      y::wvec2 normal{vec[yy], -vec[xx]};

      if (normal.dot(-mass.d) <= 0) {
        continue;
      }

      y::world ratio = get_projection(start, end, mass.v, mass.d, true);
      if (min_ratio > ratio) {
        collision = true;
        min_ratio = ratio;
        nearest_normal = normal;
      }
    }

    if (collision) {
      mass.v += y::max(0., min_ratio) * mass.d;
      return nearest_normal;
    }
    else {
      mass.v += mass.d;
    }
    return y::wvec2();
  }

}

Particle::Particle(
    y::int32 tag, y::int32 frames, y::world bounce_coefficient,
    y::world depth, y::world layering_value,
    const Derivatives<y::world>& size,
    const Derivatives<y::wvec2>& pos,
    const Derivatives<y::fvec4>& colour)
  : tag(tag)
  , frames(frames)
  , bounce_coefficient(bounce_coefficient)
  , depth(depth)
  , layering_value(layering_value)
  , size(size)
  , pos(pos)
  , colour(colour)
  , sprite(y::null)
{
}

Particle::Particle(
    y::int32 tag, y::int32 frames, y::world bounce_coefficient,
    y::world depth, y::world layering_value,
    const Derivatives<y::wvec2>& pos,
    const Derivatives<y::fvec4>& colour,
    const Sprite& sprite, const y::ivec2& frame_size, const y::ivec2& frame)
  : tag(tag)
  , frames(frames)
  , bounce_coefficient(bounce_coefficient)
  , depth(depth)
  , layering_value(layering_value)
  , pos(pos)
  , colour(colour)
  , sprite(&sprite)
  , frame_size(frame_size)
  , frame(frame)
{
}

bool Particle::update(const WorldWindow& world)
{
  if (bounce_coefficient >= 0.) {
    y::wvec2 normal = mass_collider_update(pos, world);

    // Reflect the velocity vector in the nearest geometry and apply the bounce
    // coefficient.
    if (normal != y::wvec2()) {
      normal.normalise();
      pos.d = 2 * pos.d.dot(normal) * normal - pos.d;
      pos.d *= -bounce_coefficient;
    }
  }
  else {
    pos.update();
  }

  size.update();
  colour.update();
  return --frames >= 0;
}

void Particle::modify(const Derivatives<y::wvec2>& modify)
{
  pos.v += modify.v;
  pos.d += modify.d;
  pos.d2 += modify.d2;
}

Rope::Rope(
    y::size point_masses, y::world length,
    Script* script_start, Script* script_end, 
    const y::wvec2& start, const y::wvec2& end, const params& params,
    y::world depth, y::world layering_value,
    y::world size, const y::fvec4& colour)
  : _length(point_masses <= 1 ? length : length / point_masses - 1)
  , _params(params)
  , _depth(depth)
  , _layering_value(layering_value)
  , _size(size)
  , _colour(colour)
  , _sprite(y::null)
  , _start(y::null)
  , _end(y::null)
{
  init(point_masses, script_start, script_end, start, end);
}

Rope::Rope(
    y::size point_masses, y::world length,
    Script* script_start, Script* script_end, 
    const y::wvec2& start, const y::wvec2& end, const params& params,
    y::world depth, y::world layering_value,
    const y::fvec4& colour, const Sprite& sprite,
    const y::ivec2& frame_size, const y::ivec2& frame)
  : _length(point_masses <= 1 ? length : length / point_masses - 1)
  , _params(params)
  , _depth(depth)
  , _layering_value(layering_value)
  , _colour(colour)
  , _sprite(&sprite)
  , _frame_size(frame_size)
  , _frame(frame)
  , _start(y::null)
  , _end(y::null)
{
  init(point_masses, script_start, script_end, start, end);
}

Rope::Rope(const Rope& rope)
  : _masses(rope._masses)
  , _length(rope._length)
  , _params(rope._params)
  , _depth(rope._depth)
  , _layering_value(rope._layering_value)
  , _size(rope._size)
  , _colour(rope._colour)
  , _sprite(rope._sprite)
  , _frame_size(rope._frame_size)
  , _frame(rope._frame)
  , _start(y::null)
  , _end(y::null)
{
  if (rope._start && rope._start->is_valid()) {
    _start = y::move_unique(new ScriptReference(**rope._start));
  }
  if (rope._end && rope._end->is_valid()) {
    _end = y::move_unique(new ScriptReference(**rope._end));
  }
}

Rope::~Rope()
{
}

void Rope::update(const WorldWindow& world)
{
  lock_endpoints();

  // Reset forces to zero.
  for (auto& mass : _masses) {
    mass.d2 = y::wvec2();
  }

  // Simulate springs between the masses.
  for (y::size i = 0; !_masses.empty() && i < _masses.size() - 1; ++i) {
    auto& a = _masses[i];
    auto& b = _masses[1 + i];
    y::wvec2 vec = b.v - a.v;

    y::wvec2 force;
    if (vec.length_squared()) {
      force += _params.spring_coefficient * (1 - _length / vec.length()) * vec;
    }
    force += _params.friction_coefficient * (b.d - a.d);

    // Apply forces (F = m * a).
    a.d2 += force / _params.mass;
    b.d2 -= force / _params.mass;
  }

  // Simulate the masses.
  // TODO: work out what to do about ropes getting stuck on corners.
  for (auto& mass : _masses) {
    // Apply gravity and air-friction.
    mass.d2 += _params.gravity;
    mass.d2 -= _params.air_friction * mass.d / _params.mass;

    y::wvec2 normal = mass_collider_update(mass, world);
    if (normal != y::wvec2()) {
      normal.normalise();
      y::wvec2 ground{normal[yy], -normal[xx]};

      // Project velocity onto ground direction.
      y::wvec2 project = mass.d.dot(ground) * ground;
      mass.d2 = y::wvec2();
      mass.d = (1. - _params.ground_friction) * project;
      mass_collider_update(mass, world);
    }
  }

  lock_endpoints();
}

void Rope::move(const y::wvec2& move)
{
  for (auto& mass : _masses) {
    mass.v += move;
  }
}

const Rope::mass_list& Rope::get_masses() const
{
  return _masses;
}

void Rope::init(y::size point_masses,
                Script* script_start, Script* script_end,
                const y::wvec2& start, const y::wvec2& end)
{
  if (script_start) {
    _start = y::move_unique(new ScriptReference(*script_start));
  }
  if (script_end) {
    _end = y::move_unique(new ScriptReference(*script_start));
  }
  y::wvec2 s = script_start ? script_start->get_origin() : start;
  y::wvec2 e = script_end ? script_end->get_origin() : end;

  for (y::size i = 0; i < point_masses; ++i) {
    y::wvec2 v = point_masses == 1 ? (s + e) / 2 :
        s + (i / (point_masses - 1.)) * (e - s);
    _masses.push_back({v, y::wvec2(), y::wvec2()});
  }
}

void Rope::lock_endpoints()
{
  if (_start) {
    if (!_start->is_valid()) {
      _start.reset(y::null);
    }
    else if (!_masses.empty()) {
      _masses.begin()->v = (*_start)->get_origin();
    }
  }
  if (_end) {
    if (!_end->is_valid()) {
      _end.reset(y::null);
    }
    else if (!_masses.empty()) {
      _masses.rbegin()->v = (*_end)->get_origin();
    }
  }
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

void Environment::add_particle(Particle&& particle)
{
  _particles.emplace_back(y::move(particle));
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
    y::int32 tag, const Derivatives<y::wvec2>& modify)
{
  // If iterating over the entire particle list every time we want to tweak them
  // is too much, could map tags to separate lists.
  for (Particle& p : _particles) {
    if (p.tag != tag) {
      continue;
    }
    p.modify(modify);
  }
}

void Environment::modify_particles(const Derivatives<y::wvec2>& modify)
{
  for (Particle& p : _particles) {
    p.modify(modify);
  }
}

void Environment::add_rope(const Rope& rope)
{
  _ropes.push_back(rope);
}

void Environment::add_rope(Rope&& rope)
{
  _ropes.emplace_back(y::move(rope));
}

void Environment::move_ropes(const y::wvec2& move)
{
  for (Rope& rope : _ropes) {
    rope.move(move);
  }
}

void Environment::update_physics()
{
  _particles.erase(
      y::remove_if(
          _particles.begin(), _particles.end(),
          [&](Particle& p) {return !p.update(_world);}),
      _particles.end());

  for (Rope& rope : _ropes) {
    rope.update(_world);
  }
}

void Environment::render_physics(RenderUtil& util, RenderBatch& batch) const
{
  GlUtil& gl = util.get_gl();

  gl.enable_depth(true);
  gl.enable_blend(true);

  y::size i = 0;
  auto render = [&](  
      const Sprite* sprite, const y::ivec2& frame_size, const y::ivec2& frame,
      const y::wvec2& pos, y::world depth, y::world size,
      const y::fvec4& colour)
  {
    if (sprite) {
      batch.add_sprite(sprite->texture, frame_size, false,
                       y::fvec2(pos - y::wvec2(frame_size / 2)),
                       frame, depth, 0.f, colour);
      return;
    }

    if (size <= 0) {
      return;
    }

    // Divisor buffers would be great, again.
    y::write_vector<float, y::vector<y::world>>(_pixels.data, 8 * i, {
        pos[xx] - size / 2., pos[yy] - size / 2.,
        pos[xx] + size / 2., pos[yy] - size / 2.,
        pos[xx] - size / 2., pos[yy] + size / 2.,
        pos[xx] + size / 2., pos[yy] + size / 2.});
    y::write_vector(_colour.data, 16 * i, {
        colour[rr], colour[gg], colour[bb], colour[aa],
        colour[rr], colour[gg], colour[bb], colour[aa],
        colour[rr], colour[gg], colour[bb], colour[aa],
        colour[rr], colour[gg], colour[bb], colour[aa]});
    y::write_vector<float, y::vector<y::world>>(_depth.data, 4 * i, {
        depth, depth, depth, depth});
    ++i;
  };

  for (const Particle& p : _particles) {
    render(p.sprite, p.frame_size, p.frame,
           p.pos.v, p.depth, p.size.v, p.colour.v);
  }

  for (const Rope& rope : _ropes) {
    for (const auto& mass : rope.get_masses()) {
      render(rope._sprite, rope._frame_size, rope._frame,
             mass.v, rope._depth, rope._size, rope._colour);
    }
  }

  // TODO: better line rendering.
  for (const Rope& rope : _ropes) {
    y::vector<RenderUtil::line> lines;
    const Rope::mass_list& masses = rope.get_masses();
    for (y::size i = 0; !masses.empty() && i < masses.size() - 1; ++i) {
      lines.push_back({y::fvec2(masses[i].v), y::fvec2(masses[1 + i].v)});
    }
    util.render_lines(lines, rope._colour);
  }

  _particle_program->bind();
  _particle_program->bind_attribute("pixels", _pixels.reupload());
  _particle_program->bind_attribute("colour", _colour.reupload());
  _particle_program->bind_attribute("depth", _depth.reupload());

  util.bind_pixel_uniforms(*_particle_program);
  util.quad_element(i).buffer->draw_elements(GL_TRIANGLES, 6 * i);
}

void Environment::render_physics_normal(RenderUtil& util,
                                        RenderBatch& batch) const
{
  GlUtil& gl = util.get_gl();

  gl.enable_depth(true);
  gl.enable_blend(false);

  y::size i = 0;
  auto render = [&](
      const Sprite* sprite, const y::ivec2& frame_size, const y::ivec2& frame,
      const y::wvec2& pos, y::world depth, y::world layering_value,
      y::world size)
  {
    if (sprite) {
      batch.add_sprite(sprite->normal, frame_size, true,
                       y::fvec2(pos - y::wvec2(frame_size / 2)),
                       frame, depth, 0.f, y::fvec4{1.f, 1.f, 1.f, 1.f});
      return;
    }

    if (size <= 0) {
      return;
    }

    y::write_vector<float, y::vector<y::world>>(_pixels.data, 8 * i, {
        pos[xx] - size / 2., pos[yy] - size / 2.,
        pos[xx] + size / 2., pos[yy] - size / 2.,
        pos[xx] - size / 2., pos[yy] + size / 2.,
        pos[xx] + size / 2., pos[yy] + size / 2.});
    y::write_vector<float, y::vector<y::world>>(_depth.data, 4 * i, {
        depth, depth, depth, depth});
    y::write_vector<float, y::vector<y::world>>(_layering.data, 4 * i, {
        layering_value, layering_value,
        layering_value, layering_value});
    ++i;
  };

  for (const Particle& p : _particles) {
    render(p.sprite, p.frame_size, p.frame,
           p.pos.v, p.depth, p.layering_value, p.size.v);
  }

  for (const Rope& rope : _ropes) {
    for (const auto& mass : rope.get_masses()) {
      render(rope._sprite, rope._frame_size, rope._frame,
             mass.v, rope._depth, rope._layering_value, rope._size);
    }
  }

  for (const Rope& rope : _ropes) {
    y::vector<RenderUtil::line> lines;
    const Rope::mass_list& masses = rope.get_masses();
    for (y::size i = 0; !masses.empty() && i < masses.size() - 1; ++i) {
      lines.push_back({y::fvec2(masses[i].v), y::fvec2(masses[1 + i].v)});
    }
    util.render_lines(
        lines, y::fvec4{.5f, .5f, float(rope._layering_value), 1.f});
  }

  _particle_normal_program->bind();
  _particle_normal_program->bind_attribute("pixels", _pixels.reupload());
  _particle_normal_program->bind_attribute("depth", _depth.reupload());
  _particle_normal_program->bind_attribute("layering_value",
                                           _layering.reupload());

  util.bind_pixel_uniforms(*_particle_normal_program);
  util.quad_element(i).buffer->draw_elements(GL_TRIANGLES, 6 * i);
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
