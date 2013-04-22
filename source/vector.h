#ifndef VECTOR_H
#define VECTOR_H

#include "common.h"

#include <cmath>

namespace y {

  template<y::size N>
  struct element_accessor {
    element_accessor() {}
  };

  template<typename T, y::size N>
  class vec {
  public:

    typedef vec<T, N> V;
    T elements[N];

    vec()
      : elements{0}
    {
    }

    template<typename... U,
             typename std::enable_if<N == sizeof...(U), int>::type = 0>
    vec(U... args)
      : elements{args...}
    {
    }

    vec(const T args[N])
      : elements{args}
    {
    }

    vec(const V& arg)
      : elements{}
    {
      operator=(arg);
    }

    V& operator=(const V& arg)
    {
      for (y::size i = 0; i < N; ++i) {
        elements[i] = arg.elements[i];
      }
      return *this;
    }

    T& operator[](y::size i)
    {
      return elements[i];
    }

    const T& operator[](y::size i) const
    {
      return elements[i];
    }

    template<y::size M, typename std::enable_if<(N > M), int>::type = 0>
    T& operator[](const element_accessor<M>& e)
    {
      (void)e;
      return elements[M];
    }

    template<y::size M, typename std::enable_if<(N > M), int>::type = 0>
    const T& operator[](const element_accessor<M>& e) const
    {
      (void)e;
      return elements[M];
    }

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

    V operator+(const V& arg) const
    {
      V v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = elements[i] + arg.elements[i];
      }
      return v;
    }

    V operator-(const V& arg) const
    {
      V v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = elements[i] - arg.elements[i];
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

    V operator-() const
    {
      return V().operator-(*this);
    }

    V normalised() const
    {
      return operator/(length() ? length() : 1);
    }

    V& operator+=(const V& arg)
    {
      return operator=(operator+(arg));
    }

    V& operator-=(const V& arg)
    {
      return operator=(operator-(arg));
    }

    V& operator*=(const T& arg)
    {
      return operator=(operator*(arg));
    }

    V& operator/=(const T& arg)
    {
      return operator=(operator/(arg));
    }

    V& normalise()
    {
      return operator=(normalised());
    } 

    T dot(const V& arg) const
    {
      T t = 0;
      for (y::size i = 0; i < N; ++i) {
        t += elements[i] * arg.elements[i];
      }
      return t;
    }

    T length_squared() const
    {
      return dot(*this);
    }

    T length() const
    {
      return sqrt(length_squared());
    }

    template<
      typename U = T,
      typename std::enable_if<N == 3 &&
                              std::is_same<T, U>::value, int>::type = 0>
    V cross(const V& arg) const
    {
      return V(elements[1] * arg[2] - elements[2] * arg[1],
               elements[2] * arg[0] - elements[0] * arg[2],
               elements[0] * arg[1] - elements[1] * arg[0]);
    }

    auto angle(const V& arg) const -> decltype(acos(T()))
    {
      if (!length() || !arg.length()) {
        return acos(T());
      }
      return acos(dot(arg) / (length() * arg.length()));
    }

  };

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

  typedef vec<y::int32, 2> ivec2;
  typedef vec<y::int32, 3> ivec3;
  typedef vec<y::int32, 4> ivec4;

  typedef vec<float, 2> fvec2;
  typedef vec<float, 3> fvec3;
  typedef vec<float, 4> fvec4;

}

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
}

#endif
