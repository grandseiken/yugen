#ifndef COMMON_H
#define COMMON_H

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

  typedef boost::noncopyable no_copy;

}

#endif
