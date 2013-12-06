#ifndef GAME__ENVIRONMENT_H
#define GAME__ENVIRONMENT_H

#include "../vector.h"
#include "../render/gl_handle.h"

class GlUtil;
class RenderUtil;
class WorldWindow;

// TODO: support particles which draw sprites.
struct Particle {
  Particle(y::int32 tag, y::int32 frames, y::world bounce_coefficient,
           y::world depth, y::world layering_value,
           y::world size, y::world dsize, y::world d2size,
           const y::wvec2& p, const y::wvec2& dp, const y::wvec2 d2p,
           const y::fvec4& colour, const y::fvec4& dcolour,
           const y::fvec4& d2colour);

  // Lookup tag for manipulating many particles at once (for example for wind
  // blowing snow).
  y::int32 tag;

  // Frames remaining until the particle is destroyed.
  y::int32 frames;

  // Controls how the particle interacts with world geometry. If negative, does
  // not collide with world geometry at all. Otherwise, particle bounces on
  // contact and velocity is scaled by this coefficient.
  y::world bounce_coefficient;

  y::world depth;
  y::world layering_value;

  // Size (in pixels) and derivatives.
  y::world size;
  y::world dsize;
  y::world d2size;

  // Current position of the particle and derivatives.
  y::wvec2 p;
  y::wvec2 dp;
  y::wvec2 d2p;

  // Colour and derivatives.
  y::fvec4 colour;
  y::fvec4 dcolour;
  y::fvec4 d2colour;

  bool update(const WorldWindow& world);
  void modify(
      const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add);
};

class Environment : public y::no_copy {
public:

  struct fog_params {
    y::world layering_value;
    y::wvec2 tex_offset;
    y::world frame;

    y::fvec4 colour;
    y::world fog_min;
    y::world fog_max;

    bool normal_only;
  };

  struct reflect_params {
    y::world layering_value;
    y::wvec2 tex_offset;
    y::world frame;

    y::fvec4 colour;
    y::world reflect_mix;
    y::world light_passthrough;

    y::world normal_scaling;
    y::world normal_scaling_reflect;
    y::world normal_scaling_refract;

    y::world reflect_fade_start;
    y::world reflect_fade_end;
    bool flip_x;
    bool flip_y;
    y::wvec2 flip_axes;

    y::world wave_height;
    y::world wave_scale;

    bool normal_only;
  };

  Environment(GlUtil& util, const WorldWindow& world, bool fake);

  void add_particle(const Particle& particle);

  // Destroy all particles with the given tag.
  void destroy_particles(y::int32 tag);
  void destroy_particles();

  // Add position and derivates to all particles with the given tag.
  void modify_particles(
      y::int32 tag,
      const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add);
  void modify_particles(
      const y::wvec2& p_add, const y::wvec2& dp_add, const y::wvec2& d2p_add);

  // Particle update and rendering.
  void update_particles();
  void render_particles(RenderUtil& gl) const;
  void render_particles_normal(RenderUtil& gl) const;

  // Complicated environment shaders below here.
  void render_fog_colour(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const fog_params& params) const;
  void render_fog_normal(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const fog_params& params) const;

  void render_reflect_colour(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const reflect_params& params, const GlFramebuffer& source) const;
  void render_reflect_normal(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region,
      const reflect_params& params) const;

private:

  GlUnique<GlBuffer<float, 2>> make_rect_buffer(
      RenderUtil& util, const y::wvec2& origin, const y::wvec2& region) const;

  const WorldWindow& _world;

  y::vector<Particle> _particles;
  GlDatabuffer<float, 2> _pixels;
  GlDatabuffer<float, 4> _colour;
  GlDatabuffer<float, 1> _depth;
  GlDatabuffer<float, 1> _layering;
  GlDatabuffer<GLushort, 1> _element;

  GlUnique<GlProgram> _particle_program;
  GlUnique<GlProgram> _particle_normal_program;
  GlUnique<GlProgram> _fog_program;
  GlUnique<GlProgram> _reflect_program;
  GlUnique<GlProgram> _reflect_normal_program;

  GlUnique<GlTexture2D> _f2d_256;
  GlUnique<GlTexture3D> _f3d_128;
  GlUnique<GlTexture3D> _fv23d_64;

};

#endif
