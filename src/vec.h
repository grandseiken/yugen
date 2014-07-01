#ifndef VEC_H
#define VEC_H

#include <yang/typedefs.h>
#include "common.h"

#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace proto {
  class ivec2;
}

namespace y {

template<typename T, std::size_t N>
using vec = yang::vec<T, N>;
using yang::abs;
using yang::pow;
using yang::min;
using yang::max;
using yang::euclidean_div;
using yang::euclidean_mod;

// Iterator for traversing hyperrectangles in integer-coordinate vector space.

template<typename T, std::size_t N>
class vec_iterator : public boost::iterator_facade<
    vec_iterator<T, N>,
    const vec<T, N>, boost::forward_traversal_tag> {
public:

  typedef vec<T, N> V;
  vec_iterator() : _finished(true) {}

  vec_iterator(const V& min, const V& max)
    : _min(min)
    , _size(max - min)
    , _value(min)
    , _finished(false)
  {
  }

  explicit operator bool() const
  {
    return !_finished;
  }
  
private:

  friend class boost::iterator_core_access;

  void increment()
  {
    for (std::size_t i = 0; i < N; ++i) {
      if (++_value.elements[i] < (_min + _size).elements[i]) {
        break;
      }
      _value.elements[i] = _min.elements[i];
      _finished = 1 + i == N;
    }
  }

  bool equal(const vec_iterator& arg) const
  {
    if (_finished && arg._finished) {
      return true; 
    }
    if (_finished != arg._finished) {
      return false;
    }
    return _value == arg._value;
  }

  const V& dereference() const
  {
    return _value;
  }

  V _min;
  V _size;
  V _value;
  bool _finished;

};

template<typename T, std::size_t N>
vec_iterator<T, N> cartesian(const vec<T, N>& min, const vec<T, N>& max)
{
  return vec_iterator<T, N>(min, max);
}

template<typename T, std::size_t N>
vec_iterator<T, N> cartesian(const vec<T, N>& max)
{
  return cartesian(vec<T, N>(), max);
}

// Polar coordinate functions.

template<typename T>
vec<T, 2> rotate(const vec<T, 2>& v, T angle)
{
  const vec<T, 2> row_0(cos(angle), -sin(angle));
  const vec<T, 2> row_1(sin(angle), cos(angle));
  return vec<T, 2>{v.dot(row_0), v.dot(row_1)};
}

template<typename T>
auto angle(const vec<T, 2>& v) -> decltype(atan(T()))
{
  if (!v[0] && !v[1]) {
    return 0;
  }
  return atan2(v[1], v[0]);
}

template<typename T, std::size_t N>
auto angle(const vec<T, N>& v, const vec<T, N>& u) -> decltype(acos(T()))
{
  if (!v.length() || !u.length()) {
    return acos(T());
  }
  return acos(v.dot(u) / (v.length() * u.length()));
}

template<typename T>
vec<T, 2> from_angle(T angle)
{
  return vec<T, 2>{T(cos(angle)), T(sin(angle))};
}

// Check if a vector lies in a particular region.

template<typename T, std::size_t N>
bool in_region(const vec<T, N>& v,
               const vec<T, N>& origin, const vec<T, N>& size)
{
  for (std::size_t i = 0; i < N; ++i) {
    if (v.elements[i] < origin[i] || v.elements[i] >= origin[i] + size[i]) {
      return false;
    }
  }
  return true;
}

// Perhaps this should go to a geometry.h or similar header.

template<typename T>
bool line_intersects_rect(const vec<T, 2>& start, const vec<T, 2>& end,
                          const vec<T, 2>& min, const vec<T, 2>& max)
{
  vec<T, 2> line_min = y::min(start, end);
  vec<T, 2> line_max = y::max(start, end);

  // Check bounds.
  if (!(line_min < max && line_max > min)) {
    return false;
  }

  // Check equation of line.
  if (start[0] - end[0] != 0) {
    T m = (end[1] - start[1]) / (end[0] - start[0]);
    T y_neg = end[1] + m * (min[0] - end[0]);
    T y_pos = end[1] + m * (max[0] - end[0]);

    if ((max[1] < y_neg && max[1] < y_pos) ||
        (min[1] >= y_neg && min[1] >= y_pos)) {
      return false;
    }
  }

  return true;
}

// Standard typedefs.

typedef vec<std::int32_t, 2> ivec2;
typedef vec<std::int32_t, 3> ivec3;
typedef vec<std::int32_t, 4> ivec4;
typedef vec<float, 2> fvec2;
typedef vec<float, 3> fvec3;
typedef vec<float, 4> fvec4;
typedef vec<world, 2> wvec2;
typedef vec<world, 3> wvec3;
typedef vec<world, 4> wvec4;

typedef vec_iterator<std::int32_t, 2> ivec2_iterator;
typedef vec_iterator<std::int32_t, 3> ivec3_iterator;
typedef vec_iterator<std::int32_t, 4> ivec4_iterator;
typedef vec_iterator<float, 2> fvec2_iterator;
typedef vec_iterator<float, 3> fvec3_iterator;
typedef vec_iterator<float, 4> fvec4_iterator;
typedef vec_iterator<world, 2> wvec2_iterator;
typedef vec_iterator<world, 3> wvec3_iterator;
typedef vec_iterator<world, 4> wvec4_iterator;

// Serialisation.

void save_to_proto(const y::ivec2& v, proto::ivec2& proto);
void load_from_proto(y::ivec2& v, const proto::ivec2& proto);

// End namespace y.
}

// Element accessors.

namespace {
  static const auto& xx = yang::x;
  static const auto& yy = yang::y;
  static const auto& zz = yang::z;
  static const auto& ww = yang::w;
  static const auto& rr = yang::x;
  static const auto& gg = yang::y;
  static const auto& bb = yang::z;
  static const auto& aa = yang::w;
}

// Vector hash function.

namespace std {

  template<typename T, std::size_t N>
  struct hash<y::vec<T, N>> {
    std::size_t operator()(const y::vec<T, N>& arg) const
    {
      std::hash<T> t;
      std::size_t seed = 0;
      for (std::size_t i = 0; i < N; ++i) {
        boost::hash_combine(seed, t(arg[i]));
      }
      return seed;
    }
  };

}

#endif
