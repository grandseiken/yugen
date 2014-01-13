#ifndef COMMON__FUNCTION_H
#define COMMON__FUNCTION_H

#include <functional>

namespace y {

  template<typename... T>
  using function = std::function<T...>;

  using std::bind;

  static decltype(std::placeholders::_1)& _1 = std::placeholders::_1;
  static decltype(std::placeholders::_2)& _2 = std::placeholders::_2;
  static decltype(std::placeholders::_3)& _3 = std::placeholders::_3;
  static decltype(std::placeholders::_4)& _4 = std::placeholders::_4;

}

#endif
