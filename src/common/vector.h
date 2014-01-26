#ifndef COMMON_VECTOR_H
#define COMMON_VECTOR_H

#include "math.h"

#include <vector>

namespace y {

template<typename T, typename A = std::allocator<T>>
using vector = std::vector<T, A>;

template<typename T, typename U = vector<T>>
void write_vector(vector<T>& dest, size dest_index, const U& source)
{
  size i = 0;
  for (const auto& u : source) {
    if (dest_index + i < dest.size()) {
      dest[dest_index + i] = T(u);
    }
    else {
      dest.emplace_back(T(u));
    }
    ++i;
  }
}

// End namespace y.
}

#endif
