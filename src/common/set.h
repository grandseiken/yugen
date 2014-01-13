#ifndef COMMON__SET_H
#define COMMON__SET_H

#include <unordered_set>

namespace y {

  template<typename T, typename H = std::hash<T>>
  using set = std::unordered_set<T, H>;

}

#endif
