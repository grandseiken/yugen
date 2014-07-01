#ifndef GAME_ENVIRONMENT_H
#define GAME_ENVIRONMENT_H

#include "../vec.h"
#include "../render/gl_handle.h"

class GameRenderer;
class GlUtil;
class RenderBatch;
class RenderUtil;
class Script;
class ScriptReference;
class WorldWindow;
struct Sprite;

// A value with first- and second-order derivatives.
template<typename T>
struct Derivatives {
  T v;
  T d;
  T d2;

  void update();
};

struct Particle {
  // Add a solid-colour particle.
  Particle(std::int32_t tag, std::int32_t frames, y::world bounce_coefficient,
           const Derivatives<y::wvec2>& pos,
           std::int32_t layer, y::world depth,
           const Derivatives<y::world>& size,
           const Derivatives<y::fvec4>& colour);

  // Add a textured particle.
  Particle(std::int32_t tag, std::int32_t frames, y::world bounce_coefficient,
           const Derivatives<y::wvec2>& pos,
           std::int32_t layer, y::world depth,
           const Derivatives<y::fvec4>& colour,
           const Sprite& sprite, const y::ivec2& frame_size,
           const y::ivec2& frame);

  // Lookup tag for manipulating many particles at once (for example for wind
  // blowing snow).
  std::int32_t tag;

  // Frames remaining until the particle is destroyed.
  std::int32_t frames;

  // Controls how the particle interacts with world geometry. If negative, does
  // not collide with world geometry at all. Otherwise, particle bounces on
  // contact and velocity is scaled by this coefficient.
  y::world bounce_coefficient;

  std::int32_t layer;
  y::world depth;

  // Size (in pixels), current position, colour, and derivatives. Size affects
  // rendering only, and is ignored for textured particles.
  Derivatives<y::wvec2> pos;
  Derivatives<y::world> size;
  Derivatives<y::fvec4> colour;

  // Parameters for textured particles.
  const Sprite* sprite;
  y::ivec2 frame_size;
  y::ivec2 frame;

  bool update(const WorldWindow& world);
  void modify(const Derivatives<y::wvec2>& modify);
};

class Rope {
public:

  // Various physical parameters for simulating the rope.
  struct params {
    y::world mass;
    y::world spring_coefficient;
    y::world friction_coefficient;

    y::wvec2 gravity;
    y::world air_friction;
    y::world ground_friction;
    y::world bounce_coefficient;
  };

  // Constructs a new rope. Start and end positions will be ignored if the
  // corresponding script is non-null.
  Rope(std::size_t point_masses, y::world length,
       Script* script_start, Script* script_end,
       const y::wvec2& start, const y::wvec2& end, const params& params,
       std::int32_t layer, y::world depth,
       y::world size, const y::fvec4& colour);

  // Similar for a textured Rope.
  Rope(std::size_t point_masses, y::world length,
       Script* script_start, Script* script_end,
       const y::wvec2& start, const y::wvec2& end, const params& params,
       std::int32_t layer, y::world depth,
       const y::fvec4& colour, const Sprite& sprite,
       const y::ivec2& frame_size, const y::ivec2& frame);

  Rope(const Rope& rope);
  ~Rope();

  void update(const WorldWindow& world);
  void move(const y::wvec2& move);

  typedef std::vector<Derivatives<y::wvec2>> mass_list;
  const mass_list& get_masses() const;

private:

  friend class Environment;
  void init(std::size_t point_masses,
            Script* script_start, Script* script_end,
            const y::wvec2& start, const y::wvec2& end);
  void lock_endpoints();

  mass_list _masses;
  y::world _length;
  params _params;

  // Rendering parameters work the same as for Particle (and apply to each
  // point-mass).
  std::int32_t _layer;
  y::world _depth;
  y::world _size;
  y::fvec4 _colour;

  const Sprite* _sprite;
  y::ivec2 _frame_size;
  y::ivec2 _frame;

  std::unique_ptr<ScriptReference> _start;
  std::unique_ptr<ScriptReference> _end;

};

class Environment {
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
  Environment(const Environment&) = delete;
  Environment& operator=(const Environment&) = delete;

  // Particle functions.
  void add_particle(const Particle& particle);
  void add_particle(Particle&& particle);

  // Destroy all particles with the given tag.
  void destroy_particles(std::int32_t tag);
  void destroy_particles();

  // Add position and derivates to all particles with the given tag.
  void modify_particles(
      std::int32_t tag, const Derivatives<y::wvec2>& modify);
  void modify_particles(const Derivatives<y::wvec2>& modify);

  // Rope functions.
  void add_rope(const Rope& rope);
  void add_rope(Rope&& rope);
  void move_ropes(const y::wvec2& move);

  // Particle and rope update and rendering.
  void update_physics();
  void render_physics(const GameRenderer& renderer) const;
  void render_physics_normal(const GameRenderer& renderer) const;

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
  std::vector<Particle> _particles;
  std::vector<Rope> _ropes;

  GlDatabuffer<float, 2> _pixels;
  GlDatabuffer<float, 4> _colour;
  GlDatabuffer<float, 1> _depth;
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

template<typename T>
void Derivatives<T>::update()
{
  d += d2;
  v += d;
}

#endif
