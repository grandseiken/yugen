#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/function.h"
#include "../common/math.h"
#include "../vector.h"
#include "type.h"

namespace yang {

typedef y::int32 int32;
typedef y::world world;
template<typename T, y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using vec = y::vec<T, N>;
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
  return [&](const Args&... args)
  {
    return function(arg, args...);
  };
}

// Boost's metaprogramming list implementation fakes variadic templates for
// C++03 compatibility. This makes extracting the type-list directly as a
// comma-separated type list for constructing e.g. function pointer types
// difficult.
//
// This is a real variadic type list supporting the functionality we need.
template<typename... T>
struct List {};
template<typename T, typename U>
struct Join {
  typedef List<T, U> type;
};
template<typename T, typename... U>
struct Join<T, List<U...>> {
  typedef List<T, U...> type;
};
template<typename... T, typename U>
struct Join<List<T...>, U> {
  typedef List<T..., U> type;
};
template<typename... T, typename... U>
struct Join<List<T...>, List<U...>> {
  typedef List<T..., U...> type;
};

// Templates for converting native function types into the corresponding
// Yang trampoline function types.
template<typename... Args>
struct Functions {
  typedef void (*fp_type)(Args...);
  typedef y::function<void(Args...)> f_type;
};
template<typename T>
struct FunctionsFromList {};
template<typename... Args>
struct FunctionsFromList<List<Args...>> {
  typedef typename Functions<Args...>::fp_type fp_type;
  typedef typename Functions<Args...>::f_type f_type;
};

template<typename R>
struct TrampolineReturn {
  typedef List<R*> type;
};
template<>
struct TrampolineReturn<void> {
  typedef List<> type;
};
template<typename T>
struct TrampolineReturn<vec<T, 2>> {
  typedef List<T*, T*> type;
};
template <typename T, y::size N>
struct TrampolineReturn<vec<T, N>> {
  typedef typename Join<
      T*, typename TrampolineReturn<vec<T, N - 1>>::type>::type type;
};

template<typename... Args>
struct TrampolineArgs {};
template<typename A, typename... Args>
struct TrampolineArgs<A, Args...> {
  typedef typename Join<A, typename TrampolineArgs<Args...>::type>::type type;
};
template<>
struct TrampolineArgs<> {
  typedef List<> type;
};
template<typename T, typename... Args>
struct TrampolineArgs<vec<T, 2>, Args...> {
  typedef typename TrampolineArgs<Args...>::type rest;
  typedef typename Join<List<T, T>, rest>::type type;
};
template<typename T, y::size N, typename... Args>
struct TrampolineArgs<vec<T, N>, Args...> {
  typedef typename TrampolineArgs<vec<T, N - 1>, Args...>::type rest;
  typedef typename Join<T, rest>::type type;
};

template<typename R, typename... Args>
struct TrampolineType {
  typedef typename TrampolineReturn<R>::type ret_type;
  typedef typename TrampolineArgs<Args...>::type args_type;
  typedef typename Join<ret_type, args_type>::type cat_type;
  typedef typename FunctionsFromList<cat_type>::fp_type fp_type;
  typedef typename FunctionsFromList<cat_type>::f_type f_type;
};

// Function call instrumentation for converting native types to the Yang calling
// convention for trampoline functions. TrampolineCallArgs unpacks the argument
// list; TrampolineCallReturn unpacks the return value.
template<typename... Args>
struct TrampolineCallArgs {};

// TrampolineCallArgs base case.
template<>
struct TrampolineCallArgs<> {
  typedef y::function<void()> f_type;

  void operator()(const f_type& function) const
  {
    function();
  }
};

// TrampolineCallArgs unpacking of a single primitive.
template<typename A, typename... Args>
struct TrampolineCallArgs<A, Args...> {
  typedef y::function<void(A, Args...)> f_type;

  void operator()(const f_type& function, const A& arg, const Args&... args) const
  {
    typedef TrampolineCallArgs<Args...> next_type;
    next_type()(bind_first(function, arg), args...);
  }
};

// TrampolineCallReturn for a primitive return.
template<typename R, typename... Args>
struct TrampolineCallReturn {
  typedef typename TrampolineType<R, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, R& result) const
  {
    return bind_first(function, &result); 
  }
};

// TrampolineCallReturn for a vector return.
template<typename T, y::size N, typename... Args>
struct TrampolineCallVecReturn {
  typedef typename TrampolineType<vec<T, N>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, T* result)
  {
    typedef TrampolineCallVecReturn<T, N - 1, Args...> first;
    auto f = first()(function, result);
    return bind_first(f, (N - 1) + result);
  }
};
template<typename T, typename... Args>
struct TrampolineCallVecReturn<T, 2, Args...> {
  typedef typename TrampolineType<vec<T, 2>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, T* result)
  {
    return bind_first(bind_first(function, result), 1 + result);
  }
};
template<typename T, y::size N, typename... Args>
struct TrampolineCallReturn<vec<T, N>, Args...> {
  typedef typename TrampolineType<vec<T, N>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, vec<T, N>& result)
  {
    typedef TrampolineCallVecReturn<T, N, Args...> return_type;
    return return_type()(function, result.elements);
  }
};

// Combine the above into a complete function call.
template<typename R, typename... Args>
struct TrampolineCall {
  typedef typename TrampolineType<R, Args...>::fp_type fp_type;
  typedef typename TrampolineType<R, Args...>::f_type f_type;

  R operator()(const f_type& function, const Args&... args) const
  {
    typedef TrampolineCallReturn<R, Args...> return_type;
    typedef TrampolineCallArgs<Args...> args_type;

    R result;
    auto return_bound_function = return_type()(function, result);
    args_type()(return_bound_function, args...);
    return result;
  }
};

// Specialisation for void return just to avoid declaring a variable of type
// void.
template<typename... Args>
struct TrampolineCall<void, Args...> {
  typedef typename TrampolineType<void, Args...>::fp_type fp_type;
  typedef typename TrampolineType<void, Args...>::f_type f_type;

  void operator()(const f_type& function, const Args&... args) const
  {
    typedef TrampolineCallArgs<Args...> args_type;
    args_type()(function, args...);
  }
};

// End namespace yang::internal.
}
}

#endif
