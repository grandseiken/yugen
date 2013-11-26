#ifndef PARTICLE_H
#define PARTICLE_H

#include "common.h"
#include "vector.h"
#include "render/gl_handle.h"

class GlUtil;

// TODO: move into environment class; add a tagging system for particles so that
// many can be manipulated at once (e.g. for wind blowing snow).
// TODO: allow particles to (optionally) collide with the world geometry.
// TODO: derivates for colour, also.
struct Particle {
  Particle(y::int32 frames, y::int32 size,
           const y::fvec2& p, const y::fvec2& dp, const y::fvec2 d2p,
           const y::fvec4& colour, y::world depth, y::world layering_value);

  // Frames remaining until the particle is destroyed.
  y::int32 frames;

  // Size in pixels.
  y::int32 size;

  // Current position of the particle and derivatives.
  y::fvec2 p;
  y::fvec2 dp;
  y::fvec2 d2p;

  // Drawing information.
  y::fvec4 colour;
  y::world depth;
  y::world layering_value;

  bool update();
};

class Particles : public y::no_copy {
public:

  Particles(GlUtil& gl);

  void add(const Particle& particle);
  void update();
  void render(const y::wvec2& camera) const;

private:

  const GlUtil& _gl;
  GlUnique<GlProgram> _particle_program;

  // TODO: should not be a vector. Do a uniqueid set thingy.
  y::vector<Particle> _particles;

};

#endif
