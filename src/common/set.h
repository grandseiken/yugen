#ifndef COMMON_SET_H
#define COMMON_SET_H

#include <unordered_set>

namespace y {

  template<typename T, typename H = std::hash<T>, typename P = std::equal_to<T>,
           typename A = std::allocator<T>>
  using set = std::unordered_set<T, H, P, A>;

}

#endif
