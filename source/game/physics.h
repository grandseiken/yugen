#ifndef PHYSICS_H
#define PHYSICS_H

#include "../vector.h"
#include "../common/utility.h"
#include "../common/vector.h"

class RenderUtil;

// A value with first- and second-order derivatives.
template<typename T>
struct Derivatives {
  T v;
  T d;
  T d2;

  void update();
};

class Rope : public y::no_copy {
public:

  Rope(y::size point_masses, y::world length,
       y::world mass, y::world spring, y::world friction,
       const y::wvec2& gravity, y::world air_friction,
       y::world ground_repulsion, y::world ground_friction,
       y::world ground_absorption, y::world ground_height);

  void update();
  void render(RenderUtil& util) const;

private:

  y::vector<Derivatives<y::wvec2>> _masses;

  y::world _length;
  y::world _mass;
  y::world _spring_coefficient;
  y::world _friction_coefficient;

  y::wvec2 _gravity;
  y::world _air_friction;  
  y::world _ground_repulsion;
  y::world _ground_friction;
  y::world _ground_absorption;
  y::world _ground_height;
  
};

template<typename T>
void Derivatives<T>::update()
{
  d += d2;
  v += d;
}

#endif
