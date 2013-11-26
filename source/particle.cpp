#include "particle.h"
#include "render/gl_util.h"

Particle::Particle(
    y::int32 frames, y::int32 size,
    const y::fvec2& p, const y::fvec2& dp, const y::fvec2 d2p,
    const y::fvec4& colour, y::world depth, y::world layering_value)
  : frames(frames)
  , size(size)
  , p(p)
  , dp(dp)
  , d2p(d2p)
  , colour(colour)
  , depth(depth)
  , layering_value(layering_value)
{
}

bool Particle::update()
{
  p += dp;
  dp += d2p;
  return --frames >= 0;
}

Particles::Particles(GlUtil& gl)
  : _gl(gl)
  , _particle_program(gl.make_unique_program({
        "/shaders/particle.v.glsl",
        "/shaders/particle.f.glsl"}))
{
}

void Particles::add(const Particle& particle)
{
  _particles.push_back(particle);
}

void Particles::update()
{
  // TODO: dumb.
  for (auto it = _particles.begin(); it != _particles.end();) {
    if (it->update()) {
      ++it;
    }
    else {
      it = _particles.erase(it);
    }
  }
}

void Particles::render(const y::wvec2& camera) const
{
  for (const auto& p : _particles) {
    // TODO.
  }
}
