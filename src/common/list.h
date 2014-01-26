#ifndef COMMON_LIST_H
#define COMMON_LIST_H

#include <list>

namespace y {

  template<typename T, typename A = std::allocator<T>>
  using list = std::list<T, A>;

}

#endif
