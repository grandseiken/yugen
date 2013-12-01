#ifndef COMMON__MAP_H
#define COMMON__MAP_H

#include "pair.h"
#include <unordered_map>

namespace y {

  template<typename T, typename U, typename H = std::hash<T>>
  using map = std::unordered_map<T, U, H>;

}

#endif
