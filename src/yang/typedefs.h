#ifndef YANG_SRC_TYPEDEFS_H
#define YANG_SRC_TYPEDEFS_H

#include <cstdint>
#include "../vector.h"

namespace yang {

typedef int32_t int_t;
typedef double float_t;
typedef void (*void_fp)();

template<bool B, typename T = void>
using enable_if = typename std::enable_if<B, T>::type;

template<typename T, std::size_t N, typename = yang::enable_if<(N > 1)>>
using vec = y::vec<T, N>;
template<std::size_t N, typename = yang::enable_if<(N > 1)>>
using ivec_t = y::vec<int_t, N>;
template<std::size_t N, typename = yang::enable_if<(N > 1)>>
using fvec_t = y::vec<float_t, N>;

// End namespace yang.
}

#endif
