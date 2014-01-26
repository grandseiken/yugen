#ifndef COMMON_MEMORY_H
#define COMMON_MEMORY_H

#include <cstddef>
#include <memory>

namespace y {

const std::nullptr_t null = nullptr;

template<typename T, typename D = std::default_delete<T>>
using unique = std::unique_ptr<T, D>;

using std::forward;
using std::move;

template<typename T>
auto move_unique(T* t) -> decltype(unique<T>(t))
{
  return unique<T>(t);
}

template<typename T>
auto move_unique(unique<T>& t) -> decltype(std::move(t))
{
  return std::move(t);
}

// End namespace y.
}

#endif
