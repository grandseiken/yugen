#ifndef COMMON__ORDERED_MAP_H
#define COMMON__ORDERED_MAP_H

#include "pair.h"
#include <map>

namespace y {

  template<typename T, typename U, typename L = std::less<T>>
  using ordered_map = std::map<T, U, L>;

}

#endif
