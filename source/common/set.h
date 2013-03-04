#ifndef COMMON_SET_H
#define COMMON_SET_H

#include <unordered_set>

namespace y {

  template<typename T>
  using set = std::unordered_set<T>;

};

#endif
