#ifndef COMMON__ORDERED_SET_H
#define COMMON__ORDERED_SET_H

#include <set>

namespace y {

  template<typename T, typename L = std::less<T>,
           typename A = std::allocator<T>>
  using ordered_set = std::set<T, L, A>;

}

#endif
