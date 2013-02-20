#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <memory>

namespace y {
  typedef std::size_t size;
  typedef std::string string;

  template<typename T>
  using vector = std::vector<T>;
  template<typename T>
  using unique = std::unique_ptr<T>;

  typedef vector<string> string_vector;
}

#endif
