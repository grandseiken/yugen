#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include <cstdint>
#include <cstddef>
#include <cmath>

namespace y {

typedef std::size_t size;
typedef std::intptr_t intptr;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef double world;

using std::abs;
using std::fabs;
using std::pow;

const world pi = 3.1415926535897932384626433832795028841971693993751058209749;

// End namespace y.
}

#endif
