#ifndef COMMON_H
#define COMMON_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace y {
  typedef std::size_t size;
  typedef std::string string;

  template<typename T>
  using vector = std::vector<T>;
  template<typename T>
  using set = std::unordered_set<T>;
  template<typename T, typename U>
  using map = std::unordered_map<T, U>;
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
}

#endif
