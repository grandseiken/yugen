#ifndef COMMON__MATH_H
#define COMMON__MATH_H

#include <cstdint>
#include <cstddef>
#include <cmath>

namespace y {

  typedef std::size_t size;

  typedef int8_t int8;
  typedef int16_t int16;
  typedef int32_t int32;
  typedef int64_t int64;

  typedef uint8_t uint8;
  typedef uint16_t uint16;
  typedef uint32_t uint32;
  typedef uint64_t uint64;

  typedef double world;

  using std::abs;
  using std::fabs;
  using std::pow;

  const world pi = 3.1415926535897932384626433832795028841971693993751058209749;

  template<typename T>
  inline T euclidean_div(const T& n, const T& d)
  {
    bool sign = (n < 0) == (d < 0);
    T t = (n < 0 ? -(1 + n) : n) / abs(d);
    return (d < 0) + (sign ? t : -(1 + t));
  }

  template<typename T>
  inline T euclidean_mod(const T& n, const T& d)
  {
    T num = abs(n);
    T div = abs(d);
    return (num < 0 ? num + (num % div + num / div) * div : num) % div;
  }

}

#endif
