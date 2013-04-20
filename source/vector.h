#ifndef NDMATH_H
#define NDMATH_H

#include "common.h"

#include <cmath>

namespace y {

  template<typename T, y::size N>
  struct vec_accessors {
    vec_accessors(T* elements)
      : x(*elements)
      , y(*(1 + elements))
      , z(*(2 + elements))
      , w(*(3 + elements))
    {}

    const T& x; T& y; T& z; T& w;
  };
  template<typename T>
  struct vec_accessors<T, 3> {
    vec_accessors(T* elements)
      : x(*elements)
      , y(*(1 + elements))
      , z(*(2 + elements))
    {}

    const T& x; T& y; T& z;
  };
  template<typename T>
  struct vec_accessors<T, 2> {
    vec_accessors(T* elements)
      : x(*elements)
      , y(*(1 + elements))
    {}

    const T& x; T& y;
  };
  template<typename T>
  struct vec_accessors<T, 1> {
    vec_accessors(T* elements)
      : x(*elements)
    {}

    const T& x;
  };
  template<typename T>
  struct vec_accessors<T, 0> {
    vec_accessors(T* elements) {}
  };

  template<typename T, y::size N>
  class vec : public vec_accessors<T, N> {
  public:

    typedef vec<T, N> V;
    T elements[N];

    vec()
      : vec_accessors<T ,N>(elements)
      , elements{0}
    {
    }

    template<typename... U,
             typename std::enable_if<N == sizeof...(U), int>::type = 0>
    vec(U... args)
      : vec_accessors<T, N>(elements)
      , elements{args...}
    {
    }

    vec(const T args[N])
      : vec_accessors<T, N>(elements)
      , elements{args}
    {
    }

    vec(const V& arg)
      : vec_accessors<T, N>(elements)
      , elements{}
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
      return  V(elements[1] * arg[2] - elements[2] * arg[1],
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

  typedef vec<y::int32, 2> ivec2;
  typedef vec<y::int32, 3> ivec3;
  typedef vec<y::int32, 4> ivec4;

  typedef vec<float, 2> fvec2;
  typedef vec<float, 3> fvec3;
  typedef vec<float, 4> fvec4;

}

#endif
