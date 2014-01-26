#ifndef COMMON_ALGORITHM_H
#define COMMON_ALGORITHM_H

#include "math.h"
#include <algorithm>
#include <iterator>

namespace y {

using std::swap;
using std::sort;
using std::stable_sort;
using std::copy;
using std::copy_if;
using std::remove_if;
using std::back_inserter;

template<typename T>
using less = std::less<T>;

template<typename T>
inline const T& max(const T& a, const T& b)
{
  return std::max(a, b);
}

template<typename T>
inline const T& min(const T& a, const T& b)
{
  return std::min(a, b);
}

template<typename T>
inline void clamp(T& t, T start, T end)
{
  t = max(start, min(end - 1, t));
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
  return b > a ? min(b - a, 2 * pi + a - b) : min(a - b, 2 * pi + b - a);
}

// End namespace y.
}

#endif
