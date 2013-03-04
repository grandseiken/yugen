#ifndef COMMON_POINTER_H
#define COMMON_POINTER_H

#include <memory>

namespace y {

  const std::nullptr_t null = nullptr;

  template<typename T>
  using unique = std::unique_ptr<T>;

  template<typename T>
  auto move_unique(T* t) -> decltype(unique<T>(t))
  {
    return unique<T>(t);
  }

}

#endif
