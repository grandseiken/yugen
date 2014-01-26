#ifndef COMMON_FUNCTION_H
#define COMMON_FUNCTION_H

#include <functional>

namespace y {

  typedef void (*void_fp)();

  template<typename T>
  using function = std::function<T>;

  using std::bind;

  static const auto& _1 = std::placeholders::_1;
  static const auto& _2 = std::placeholders::_2;
  static const auto& _3 = std::placeholders::_3;
  static const auto& _4 = std::placeholders::_4;

}

#endif
