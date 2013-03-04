#ifndef COMMON_MAP_H
#define COMMON_MAP_H

#include <unordered_map>

namespace y {

  template<typename T, typename U>
  using map = std::unordered_map<T, U>;

  template<typename T, typename U>
  using pair = std::pair<T, U>;

  template<typename T, typename U>
  pair<T, U> make_pair(T t, U u)
  {
    return pair<T, U>(std::forward<T>(t), std::forward<U>(u));
  }

}

#endif
