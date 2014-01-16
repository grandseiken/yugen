#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/math.h"
#include "../vector.h"
#include "type.h"

namespace yang {

typedef y::int32 int32;
typedef y::world world;
template<y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using int32_vec = y::vec<int32, N>;
template<y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using world_vec = y::vec<world, N>;

namespace internal {

struct TypeInfoHelper {
  static yang::Type int32_vector_representation(y::size n);
  static yang::Type world_vector_representation(y::size n);
};

template<typename T>
struct TypeInfo {};
// TODO: TypeInfo for user types and function types.
// Function types need some careful design to handle the implicit global data
// parameter (at least if we want to make them callable...).

template<>
struct TypeInfo<void> {
  yang::Type representation() const
  {
    yang::Type t;
    return t;
  }
};

template<>
struct TypeInfo<yang::int32> {
  yang::Type representation() const
  {
    yang::Type t;
    t._base = yang::Type::INT;
    return t;
  }
};

template<>
struct TypeInfo<yang::world> {
  yang::Type representation() const
  {
    yang::Type t;
    t._base = yang::Type::WORLD;
    return t;
  }
};

// TODO: vector TypeInfo doesn't work; not sure what the calling conventions
// for vectors are. Or if they exist at all. If not, will need to convert
// functions with vector arguments to value-structs of some kind. It'd be
// nice to only do that for export functions, but without additional machinery
// the call sites wouldn't know which convention to use.
// Supporting first-class interop of vectors with y::vecs is a must-have
// feature.
template<y::size N>
struct TypeInfo<int32_vec<N>> {
  yang::Type representation() const
  {
    yang::Type t;
    t._base = yang::Type::INT;
    t._count = N;
    return t;
  }
};

template<y::size N>
struct TypeInfo<world_vec<N>> {
  yang::Type representation() const
  {
    yang::Type t;
    t._base = yang::Type::WORLD;
    t._count = N;
    return t;
  }
};

// End namespace yang::internal.
}
}

#endif
