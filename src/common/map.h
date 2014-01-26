#ifndef COMMON_MAP_H
#define COMMON_MAP_H

#include "pair.h"
#include <unordered_map>

namespace y {

  template<typename T, typename U, typename H = std::hash<T>,
           typename P = std::equal_to<T>, typename A = std::allocator<T>>
  using map = std::unordered_map<T, U, H, P, A>;

}

#endif
