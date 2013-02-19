#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

namespace y {
  typedef std::size_t size;
  typedef std::string string;

  template<typename T>
  using vector = std::vector<T>;

  typedef vector<string> string_vector;
}

#endif
