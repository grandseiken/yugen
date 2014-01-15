#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/math.h"

namespace yang {

class Type;
typedef y::int32 int32;
typedef y::world world;

namespace internal {

// TODO: TypeInfo for all types amd user types.
template<typename T>
struct TypeInfo {};

template<>
struct TypeInfo<void> {
  yang::Type representation() const;
};

template<>
struct TypeInfo<yang::int32> {
  yang::Type representation() const;
};

template<>
struct TypeInfo<yang::world> {
  yang::Type representation() const;
};

// End namespace yang::internal.
}
}

#endif
