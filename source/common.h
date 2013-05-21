#ifndef COMMON_H
#define COMMON_H

#include <boost/functional/hash.hpp>
#include <boost/utility.hpp>
#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// TODO: do we need all this here?
namespace y {

  typedef int8_t int8;
  typedef int16_t int16;
  typedef int32_t int32;
  typedef int64_t int64;

  typedef uint8_t uint8;
  typedef uint16_t uint16;
  typedef uint32_t uint32;
  typedef uint64_t uint64;

  typedef std::size_t size;
  typedef std::string string;
  typedef std::stringstream sstream;
  typedef std::ostream ostream;
  typedef std::istream istream;

  template<typename T>
  T abs(const T& t)
  {
    return std::abs(t);
  }
  template<typename T>
  T min(const T& a, const T& b)
  {
    return std::min(a, b);
  }
  template<typename T>
  T max(const T& a, const T& b)
  {
    return std::max(a, b);
  }
  template<typename T>
  T euclidean_div(const T& n, const T& d)
  {
    bool sign = (n < 0) == (d < 0);
    T t = (n < 0 ? -(1 + n) : n) / abs(d);
    return (d < 0) + (sign ? t : -(1 + t));
  }
  template<typename T>
  T euclidean_mod(const T& n, const T& d)
  {
    T div = abs(d);
    return (n < 0 ? n * (1 - div) : n) % div;
  }
  template<typename T>
  void clamp(T& t, T start, T end)
  {
    t = max(start, min(end - 1, t));
  }
  template<typename T>
  void roll(T& t, T start, T end)
  {
    while (t < 0) {
      t += end - start;
    }
    t = start + (t - start) % (end - start);
  }

  template<typename T>
  using vector = std::vector<T>;
  template<typename T, typename H = std::hash<T>>
  using set = std::unordered_set<T, H>;
  template<typename T, typename U, typename H = std::hash<T>>
  using map = std::unordered_map<T, U, H>;
  template<typename T, typename L = std::less<T>>
  using ordered_set = std::set<T, L>;
  template<typename T, typename U, typename L = std::less<T>>
  using ordered_map = std::map<T, U, L>;
  template<typename T, typename U>
  using pair = std::pair<T, U>;
  template<typename T>
  using unique = std::unique_ptr<T>;

  typedef vector<string> string_vector;
  const std::nullptr_t null = nullptr;

  template<typename T>
  auto move(T t) -> decltype(std::move(std::forward<T>(t)))
  {
    return std::move(std::forward<T>(t));
  }

  template<typename T>
  auto move_unique(T* t) -> decltype(unique<T>(t))
  {
    return unique<T>(t);
  }

  template<typename T>
  auto move_unique(unique<T>& t) -> decltype(std::move(t))
  {
    return std::move(t);
  }

  template<typename T, typename U>
  pair<T, U> make_pair(T t, U u)
  {
    return pair<T, U>(std::forward<T>(t), std::forward<U>(u));
  }

  // TODO: should we use some sort of y::angle class? Define it in vector.h?
  typedef boost::noncopyable no_copy;
  typedef double world;
  const world pi = 3.1415926535897932384626433832795028841971693993751058209749;
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

}

namespace std {

  template<typename T, typename U>
  struct hash<y::pair<T, U>> {
    y::size operator()(const y::pair<T, U>& arg) const
    {
      std::hash<T> t;
      std::hash<U> u;

      y::size seed = 0;
      boost::hash_combine(seed, u(arg.first));
      boost::hash_combine(seed, t(arg.second));
      return seed;
    }
  };

}

#endif
