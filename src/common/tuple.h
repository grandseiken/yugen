#ifndef COMMON__TUPLE_H
#define COMMON__TUPLE_H

#include "math.h"
#include <tuple>

namespace y {

  template<typename... T>
  using tuple = std::tuple<T...>;
  template<y::size I, typename T>
  using tuple_elem = std::tuple_element<I, T>;
  template<typename T>
  using tuple_size = std::tuple_size<T>;

  using std::get;

}

#endif
