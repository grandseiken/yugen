#ifndef COMMON_H
#define COMMON_H

#include <algorithm>

namespace y {

typedef double world;
const world pi = 3.1415926535897932384626433832795028841971693993751058209749;

template<typename T>
inline void clamp(T& t, T start, T end)
{
  t = std::max(start, std::min(end - 1, t));
}

template<typename T>
inline void roll(T& t, T start, T end)
{
  while (t < 0) {
    t += end - start;
  }
  t = start + (t - start) % (end - start);
}

// TODO: should we use some sort of y::angle class? Define it in vector.h?
inline world angle(world a)
{
  while (a > pi) {
    a -= 2 * pi;
  }
  while (a < -pi) {
    a += 2 * pi;
  }
  return a;
}

inline world angle_distance(world a, world b)
{
  a = angle(a);
  b = angle(b);
  return b > a ?
      std::min(b - a, 2 * pi + a - b) : std::min(a - b, 2 * pi + b - a);
}

// End namespace y.
}

#endif
