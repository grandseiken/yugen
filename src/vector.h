#ifndef VECTOR_H
#define VECTOR_H

#include "common/algorithm.h"
#include "common/math.h"
#include "common/io.h"
#include "common/utility.h"

#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace proto {
  class ivec2;
}

namespace y {

template<y::size N>
struct element_accessor {
  element_accessor() {}
};

// N-dimensional mathematical vector over elements of type T.
template<typename T, y::size N>
class vec {
public:

  typedef vec V;
  T elements[N];

  // Constructors.

  vec()
    : elements{0}
  {
  }

  template<typename... U,
           typename = y::enable_if<N == sizeof...(U)>>
  vec(U... args)
    : elements{args...}
  {
  }

  vec(const T args[N])
    : elements{args}
  {
  }

  vec(const V& arg) noexcept = default;
  vec(V&& arg) noexcept = default;

  template<typename U>
  explicit vec(const vec<U, N>& arg)
    : elements{}
  {
    for (y::size i = 0; i < N; ++i) {
      elements[i] = T(arg.elements[i]);
    }
  }

  // Assignment.

  V& operator=(const V& arg) noexcept = default;
  V& operator=(V&& arg) noexcept = default;

  // Indexing.

  T& operator[](y::size i)
  {
    return elements[i];
  }

  const T& operator[](y::size i) const
  {
    return elements[i];
  }

  template<y::size M, typename = y::enable_if<(N > M)>>
  T& operator[](const element_accessor<M>&)
  {
    return elements[M];
  }

  template<y::size M, typename = y::enable_if<(N > M)>>
  const T& operator[](const element_accessor<M>&) const
  {
    return elements[M];
  }

  // Comparison operators. Relational operators return true iff all of the
  // element-wise comparisons return true.

  bool operator==(const V& arg) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] != arg.elements[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const V& arg) const
  {
    return !operator==(arg);
  }

  bool operator<(const V& arg) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] >= arg.elements[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator>(const V& arg) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] <= arg.elements[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator<=(const V& arg) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] > arg.elements[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator>=(const V& arg) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] < arg.elements[i]) {
        return false;
      }
    }
    return true;
  }

  // Binary operators.

  V operator+(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] + arg[i];
    }
    return v;
  }

  V operator-(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] - arg[i];
    }
    return v;
  }

  V operator*(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] * arg[i];
    }
    return v;
  }

  V operator/(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] / arg[i];
    }
    return v;
  }

  V operator*(const T& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] * arg;
    }
    return v;
  }

  V operator/(const T& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = elements[i] / arg;
    }
    return v;
  }

  // Shortcut assignment operators.

  V& operator+=(const V& arg)
  {
    return operator=(operator+(arg));
  }

  V& operator-=(const V& arg)
  {
    return operator=(operator-(arg));
  }

  V& operator*=(const V& arg)
  {
    return operator=(operator*(arg));
  }

  V& operator/=(const V& arg)
  {
    return operator=(operator/(arg));
  }

  V& operator*=(const T& arg)
  {
    return operator=(operator*(arg));
  }

  V& operator/=(const T& arg)
  {
    return operator=(operator/(arg));
  }

  // Unary operators and mathematical functions.

  V operator-() const
  {
    return V().operator-(*this);
  }

  V normalised() const
  {
    return operator/(length() ? length() : 1);
  }

  V& normalise()
  {
    return operator=(normalised());
  }

  T length_squared() const
  {
    return dot(*this);
  }

  T length() const
  {
    return sqrt(length_squared());
  }

  // Dot product.

  T dot(const V& arg) const
  {
    T t = 0;
    for (y::size i = 0; i < N; ++i) {
      t += elements[i] * arg.elements[i];
    }
    return t;
  }

  // Cross productions for two and three dimensions.

  template<
      typename U = T,
      typename = y::enable_if<N == 2 &&
                              y::is_same<T, U>::value>>
  T cross(const V& arg) const
  {
    return elements[0] * arg[1] - arg[0] * elements[1];
  }

  template<
      typename U = T,
      typename = y::enable_if<N == 3 &&
                              y::is_same<T, U>::value>>
  V cross(const V& arg) const
  {
    return V(elements[1] * arg[2] - elements[2] * arg[1],
             elements[2] * arg[0] - elements[0] * arg[2],
             elements[0] * arg[1] - elements[1] * arg[0]);
  }

  // Polar coordinate functions.

  template<
      typename U = T,
      typename = y::enable_if<N == 2 &&
                              y::is_same<T, U>::value>>
  V rotate(T angle) const
  {
    const V row_0(cos(angle), -sin(angle));
    const V row_1(sin(angle), cos(angle));
    return V{dot(row_0), dot(row_1)};
  }

  template<
      typename U = T,
      typename = y::enable_if<N == 2 &&
                              y::is_same<T, U>::value>>
  auto angle() const -> decltype(atan(T()))
  {
    if (!elements[0] && !elements[1]) {
      return 0;
    }
    return atan2(elements[1], elements[0]);
  }

  auto angle(const V& arg) const -> decltype(acos(T()))
  {
    if (!length() || !arg.length()) {
      return acos(T());
    }
    return acos(dot(arg) / (length() * arg.length()));
  }

  template<
      typename U = T,
      typename = y::enable_if<N == 2 &&
                              y::is_same<T, U>::value>>
  static V from_angle(T angle)
  {
    return V{T(cos(angle)), T(sin(angle))};
  }

  // Euclidean division and modulo operators.

  V euclidean_div(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = y::euclidean_div(elements[i], arg[i]);
    }
    return v;
  }

  V euclidean_mod(const V& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = y::euclidean_mod(elements[i], arg[i]);
    }
    return v;
  }

  V euclidean_div(const T& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = y::euclidean_div(elements[i], arg);
    }
    return v;
  }

  V euclidean_mod(const T& arg) const
  {
    V v;
    for (y::size i = 0; i < N; ++i) {
      v[i] = y::euclidean_mod(elements[i], arg);
    }
    return v;
  }

  // Check if a vector lies in a particular region.

  bool in_region(const V& origin, const V& size) const
  {
    for (y::size i = 0; i < N; ++i) {
      if (elements[i] < origin[i] || elements[i] >= origin[i] + size[i]) {
        return false;
      }
    }
    return true;
  }

};

// Binary operators with a single element on the left.

template<typename T, y::size N>
vec<T, N> operator*(const T& t, const vec<T, N>& v)
{
  vec<T, N> u;
  for (y::size i = 0; i < N; ++i) {
    u[i] = t * v[i];
  }
  return u;
}

template<typename T, y::size N>
vec<T, N> operator/(const T& t, const vec<T, N>& v)
{
  vec<T, N> u;
  for (y::size i = 0; i < N; ++i) {
    u[i] = t / v[i];
  }
  return u;
}

// Miscellaenous element-wise mathematical functions.

template<typename T, y::size N>
vec<T, N> abs(const vec<T, N>& v)
{
  vec<T, N> u;
  for (y::size i = 0; i < N; ++i) {
    u[i] = abs(v[i]);
  }
  return u;
}

template<typename T, y::size N>
vec<T, N> pow(const T& a, const vec<T, N>& b)
{
  vec<T, N> v;
  for (y::size i = 0; i < N; ++i) {
    v[i] = pow(a, b[i]);
  }
  return v;
}

template<typename T, y::size N>
vec<T, N> pow(const vec<T, N>& a, const T& b)
{
  vec<T, N> v;
  for (y::size i = 0; i < N; ++i) {
    v[i] = pow(a[i], b);
  }
  return v;
}

template<typename T, y::size N>
vec<T, N> pow(const vec<T, N>& a, const vec<T, N>& b)
{
  vec<T, N> v;
  for (y::size i = 0; i < N; ++i) {
    v[i] = pow(a[i], b[i]);
  }
  return v;
}

template<typename T, y::size N>
vec<T, N> min(const vec<T, N>& a, const vec<T, N>& b)
{
  vec<T, N> v;
  for (y::size i = 0; i < N; ++i) {
    v[i] = min(a[i], b[i]);
  }
  return v;
}

template<typename T, y::size N>
vec<T, N> max(const vec<T, N>& a, const vec<T, N>& b)
{
  vec<T, N> v;
  for (y::size i = 0; i < N; ++i) {
    v[i] = max(a[i], b[i]);
  }
  return v;
}

// Writing string representation to stream.

template<typename T, y::size N>
y::ostream& operator<<(y::ostream& o, const vec<T, N>& arg)
{
  for (y::size i = 0; i < N; ++i) {
    if (i) {
      o << ", ";
    }
    o << arg.elements[i];
  }
  return o;
}

// Iterator for traversing hyperrectangles in integer-coordinate vector space.

template<typename T, y::size N>
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
    for (y::size i = 0; i < N; ++i) {
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

template<typename T, y::size N>
vec_iterator<T, N> cartesian(const vec<T, N>& min, const vec<T, N>& max)
{
  return vec_iterator<T, N>(min, max);
}

template<typename T, y::size N>
vec_iterator<T, N> cartesian(const vec<T, N>& max)
{
  return cartesian(vec<T, N>(), max);
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

typedef vec<int32, 2> ivec2;
typedef vec<int32, 3> ivec3;
typedef vec<int32, 4> ivec4;
typedef vec<float, 2> fvec2;
typedef vec<float, 3> fvec3;
typedef vec<float, 4> fvec4;
typedef vec<world, 2> wvec2;
typedef vec<world, 3> wvec3;
typedef vec<world, 4> wvec4;

typedef vec_iterator<int32, 2> ivec2_iterator;
typedef vec_iterator<int32, 3> ivec3_iterator;
typedef vec_iterator<int32, 4> ivec4_iterator;
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

// Standard accessor types for 4-element vectors.

template<bool>
struct element_accessor_instance {
  static const y::element_accessor<0> x;
  static const y::element_accessor<1> y;
  static const y::element_accessor<2> z;
  static const y::element_accessor<3> w;
};

namespace {
  static const y::element_accessor<0>& xx = element_accessor_instance<true>::x;
  static const y::element_accessor<1>& yy = element_accessor_instance<true>::y;
  static const y::element_accessor<2>& zz = element_accessor_instance<true>::z;
  static const y::element_accessor<3>& ww = element_accessor_instance<true>::w;
  static const y::element_accessor<0>& rr = element_accessor_instance<true>::x;
  static const y::element_accessor<1>& gg = element_accessor_instance<true>::y;
  static const y::element_accessor<2>& bb = element_accessor_instance<true>::z;
  static const y::element_accessor<3>& aa = element_accessor_instance<true>::w;
}

// Vector hash function.

namespace std {

  template<typename T, y::size N>
  struct hash<y::vec<T, N>> {
    y::size operator()(const y::vec<T, N>& arg) const
    {
      std::hash<T> t;
      y::size seed = 0;
      for (y::size i = 0; i < N; ++i) {
        boost::hash_combine(seed, t(arg[i]));
      }
      return seed;
    }
  };

}

#endif
