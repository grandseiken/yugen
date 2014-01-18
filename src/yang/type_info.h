#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/function.h"
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

// Standard bind function requires explicit placeholders for all elements, which
// makes it unusable in the variadic setting. We only need to bind one argument
// at a time.
template<typename R, typename A, typename... Args>
y::function<R(Args...)> bind_first(
    const y::function<R(A, Args...)>& function, const A& arg)
{
  return [=](Args... args)
  {
    return function(arg, args...);
  };
}

// Function call instrumentation for converting native types to the Yang calling
// convention.
template<typename R, typename... Args>
struct TrampolineCall {};

template<typename R>
struct TrampolineCall<R> {
  typedef R (*fp_type)();
  typedef y::function<R()> f_type;

  R operator()(fp_type function) const
  {
    return function();
  }

  R operator()(const f_type& function) const
  {
    return function();
  }
};

template<typename R, typename A, typename... Args>
struct TrampolineCall<R, A, Args...> {
  typedef R (*fp_type)(A, Args...);
  typedef y::function<R(A, Args...)> f_type;

  R operator()(fp_type function, const A& arg, const Args&... args) const
  {
    return operator()(f_type(function), arg, args...);
  }

  R operator()(const f_type& function, const A& arg, const Args&... args) const
  {
    typedef TrampolineCall<R, Args...> next_type;
    next_type next;
    return next(bind_first(function, arg), args...);
  }
};

// End namespace yang::internal.
}
}

#endif
