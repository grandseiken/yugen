#include "physics.h"
#include "../render/util.h"

Rope::Rope(y::size point_masses, y::world length,
     y::world mass, y::world spring, y::world friction,
     const y::wvec2& gravity, y::world air_friction,
     y::world ground_repulsion, y::world ground_friction,
     y::world ground_absorption, y::world ground_height)
  : _length(length)
  , _mass(mass)
  , _spring_coefficient(spring)
  , _friction_coefficient(friction)
  , _gravity(gravity)
  , _air_friction(air_friction)
  , _ground_repulsion(ground_repulsion)
  , _ground_friction(ground_friction)
  , _ground_absorption(ground_absorption)
  , _ground_height(ground_height)
{
  for (y::size i = 0; i < point_masses; ++i) {
    _masses.push_back({y::wvec2{i * length, 0.}, y::wvec2(), y::wvec2()});
  }
}

void Rope::update()
{
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
      force += _spring_coefficient * (1 - _length / vec.length()) * vec;
    }
    force += _friction_coefficient * (b.d - a.d);

    // Apply forces (F = m * a).
    a.d2 += force / _mass;
    b.d2 -= force / _mass;
  }

  // Simulate the masses.
  for (auto& mass : _masses) {
    // Apply gravity and air-friction.
    mass.d2 += _gravity;
    mass.d2 -= _air_friction * mass.d / _mass;

    // TODO: proper collision.
    if (mass.v[yy] > _ground_height) {
      // Slide along the "ground".
      mass.d2 -= _ground_friction * y::wvec2{mass.d[xx], 0.} / _mass;

      // Absorb velocity.
      if (mass.d[yy] > 0) {
        mass.d2 -= _ground_absorption * y::wvec2{0., mass.d[yy]} / _mass;
      }

      // Repulse velocity.
      mass.d2 += _ground_repulsion *
          y::wvec2{0., _ground_height - mass.v[yy]} / _mass;
    }

    mass.update();
  }
}

void Rope::render(RenderUtil& util) const
{
  y::vector<RenderUtil::line> lines;
  for (y::size i = 0; !_masses.empty() && i < _masses.size() - 1; ++i) {
    lines.push_back({y::fvec2(_masses[i].v), y::fvec2(_masses[1 + i].v)});
  }
  util.render_lines(lines, colour::white);
}
