#ifndef VECTOR_H
#define VECTOR_H

#include "common.h"

#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <cmath>

namespace proto {
  class ivec2;
}

namespace y {

  template<y::size N>
  struct element_accessor {
    element_accessor() {}
  };

  template<typename T, y::size N, bool non_strict = true>
  class vec {
  public:

    typedef vec<T, N, non_strict> V;
    T elements[N];

    vec()
      : elements{0}
    {
    }

    template<typename... U,
             typename std::enable_if<N == sizeof...(U), bool>::type = 0>
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

    vec(V&& arg) noexcept = default;

    template<typename U>
    explicit vec(const vec<U, N>& arg)
      : elements{}
    {
      for (y::size i = 0; i < N; ++i) {
        elements[i] = T(arg.elements[i]);
      }
    }

    V& operator=(const V& arg)
    {
      for (y::size i = 0; i < N; ++i) {
        elements[i] = arg.elements[i];
      }
      return *this;
    }

    V& operator=(V&& arg) noexcept = default;

    T& operator[](y::size i)
    {
      return elements[i];
    }

    const T& operator[](y::size i) const
    {
      return elements[i];
    }

    template<y::size M, typename std::enable_if<(N > M), bool>::type = 0>
    T& operator[](const element_accessor<M>& e)
    {
      (void)e;
      return elements[M];
    }

    template<y::size M, typename std::enable_if<(N > M), bool>::type = 0>
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

    template<typename std::enable_if<non_strict, bool>::type = 0>
    V operator*(const V& arg) const
    {
      V v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = elements[i] * arg[i];
      }
      return v;
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    V operator/(const V& arg) const
    {
      V v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = elements[i] / arg[i];
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

    template<typename std::enable_if<non_strict, bool>::type = 0>
    V& operator*=(const V& arg)
    {
      return operator=(operator*(arg));
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    V& operator/=(const V& arg)
    {
      return operator=(operator/(arg));
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    bool operator<(const V& arg) const
    {
      for (y::size i = 0; i < N; ++i) {
        if (elements[i] >= arg.elements[i]) {
          return false;
        }
      }
      return true;
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    bool operator>(const V& arg) const
    {
      for (y::size i = 0; i < N; ++i) {
        if (elements[i] <= arg.elements[i]) {
          return false;
        }
      }
      return true;
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    bool operator<=(const V& arg) const
    {
      for (y::size i = 0; i < N; ++i) {
        if (elements[i] > arg.elements[i]) {
          return false;
        }
      }
      return true;
    }

    template<typename std::enable_if<non_strict, bool>::type = 0>
    bool operator>=(const V& arg) const
    {
      for (y::size i = 0; i < N; ++i) {
        if (elements[i] < arg.elements[i]) {
          return false;
        }
      }
      return true;
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
        typename std::enable_if<N == 2 &&
                                std::is_same<T, U>::value, bool>::type = 0>
    T cross(const V& arg) const
    {
      return elements[0] * arg[1] - arg[0] * elements[1];
    }

    template<
        typename U = T,
        typename std::enable_if<N == 3 &&
                                std::is_same<T, U>::value, bool>::type = 0>
    V cross(const V& arg) const
    {
      return V(elements[1] * arg[2] - elements[2] * arg[1],
               elements[2] * arg[0] - elements[0] * arg[2],
               elements[0] * arg[1] - elements[1] * arg[0]);
    }

    template<
        typename U = T,
        typename std::enable_if<N == 2 &&
                                std::is_same<T, U>::value, bool>::type = 0>
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
        typename std::enable_if<N == 2 &&
                                std::is_same<T, U>::value, bool>::type = 0>
    static V from_angle(T angle)
    {
      return V{T(cos(angle)), T(sin(angle))};
    }

    bool in_region(const V& origin, const V& size) const
    {
      for (y::size i = 0; i < N; ++i) {
        if (elements[i] < origin[i] || elements[i] >= origin[i] + size[i]) {
          return false;
        }
      }
      return true;
    }
 
    V euclidean_div(const V& arg) const
    {
      vec<T, N> v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = y::euclidean_div(elements[i], arg[i]);
      }
      return v;
    }

    V euclidean_mod(const V& arg) const
    {
      vec<T, N> v;
      for (y::size i = 0; i < N; ++i) {
        v[i] = y::euclidean_mod(elements[i], arg[i]);
      }
      return v;
    }

  };

  template<typename T, y::size N, bool non_strict>
  vec<T, N, non_strict> operator*(const T& t, const vec<T, N, non_strict>& v)
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

  template<typename T, y::size N, bool non_strict>
  y::ostream& operator<<(y::ostream& o, const vec<T, N, non_strict>& arg)
  {
    for (y::size i = 0; i < N; ++i) {
      if (i) {
        o << ", ";
      }
      o << arg.elements[i];
    }
    return o;
  }

  template<typename T, y::size N, bool non_strict>
  class vec_iterator : public boost::iterator_facade<
      vec_iterator<T, N, non_strict>,
      const vec<T, N, non_strict>, boost::forward_traversal_tag> {
  public:

    typedef vec<T, N, non_strict> V;
    vec_iterator() : _finished(true) {}

    vec_iterator(const V& min, const V& max)
      : _min(min)
      , _size(max - min)
      , _value(min)
      , _finished(false)
    {
    }

    vec_iterator<T, N, non_strict> end()
    {
      return vec_iterator();
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

  template<typename T, y::size N, bool non_strict>
  vec_iterator<T, N, non_strict> cartesian(const vec<T, N, non_strict>& min,
                                           const vec<T, N, non_strict>& max)
  {
    return vec_iterator<T, N, non_strict>(min, max);
  }

  template<typename T, y::size N, bool non_strict>
  vec_iterator<T, N, non_strict> cartesian(const vec<T, N, non_strict>& max)
  {
    return cartesian(vec<T, N, non_strict>(), max);
  }

  typedef vec<int32, 2> ivec2;
  typedef vec<int32, 3> ivec3;
  typedef vec<float, 2> fvec2;
  typedef vec<float, 4> fvec4;
  typedef vec<world, 2> wvec2;

  typedef vec_iterator<int32, 2, true> ivec2_iterator;
  typedef vec_iterator<float, 2, true> fvec2_iterator;
  typedef vec_iterator<float, 4, true> fvec4_iterator;
  typedef vec_iterator<world, 4, true> wvec2_iterator;

  void save_to_proto(const y::ivec2& v, proto::ivec2& proto);
  void load_from_proto(y::ivec2& v, const proto::ivec2& proto);

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
  static const y::element_accessor<0>& rr = element_accessor_instance<true>::x;
  static const y::element_accessor<1>& gg = element_accessor_instance<true>::y;
  static const y::element_accessor<2>& bb = element_accessor_instance<true>::z;
  static const y::element_accessor<3>& aa = element_accessor_instance<true>::w;
}

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
