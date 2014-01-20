#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/function.h"
#include "../common/math.h"
#include "../vector.h"
#include "type.h"

namespace yang {

class Instance;
namespace internal {
  // TODO: TypeInfo for user types.
  template<typename T>
  struct TypeInfo;
  template<typename T>
  struct ValueConstruct;

  template<typename... Args>
  struct TrampolineCallArgs;
  template<typename R, typename... Args>
  struct TrampolineCallReturn;
  template<typename R, typename... Args>
  struct TrampolineCall;
}

typedef y::int32 int32;
typedef y::world world;
template<typename T, y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using vec = y::vec<T, N>;
template<y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using int32_vec = y::vec<int32, N>;
template<y::size N, typename y::enable_if<(N > 1), bool>::type = 0>
using world_vec = y::vec<world, N>;

// Opaque Yang function object.
template<typename T>
class Function {
  static_assert(sizeof(T) != sizeof(T), "use of non-function type");
};

template<typename R, typename... Args>
class Function<R(Args...)> {
public:

  // Get the program instance that this function references.
  Instance& get_instance() const;
  // Get the type corresponding to this function type as a yang Type object.
  static Type get_type();

  // True if the object references a non-null function. It is an error to pass
  // a null function object to a yang program instance, or invoke it.
  explicit operator bool() const;

  // Invoke the function.
  R operator()(const Args&... args) const;

private:

  template<typename... CArgs>
  friend class internal::TrampolineCallArgs;
  template<typename CR, typename... CArgs>
  friend class internal::TrampolineCallReturn;
  template<typename CR, typename... CArgs>
  friend class internal::TrampolineCall;

  template<typename T>
  friend class internal::ValueConstruct;
  template<typename T>
  friend class Function;
  friend class Instance;

  Function(Instance& instance)
    : _function(y::null)
    , _instance(&instance) {}

  y::void_fp _function;
  Instance* _instance;

};

template<typename R, typename... Args>
Instance& Function<R(Args...)>::get_instance() const
{
  return *_instance;
}

template<typename R, typename... Args>
Type Function<R(Args...)>::get_type()
{
  internal::TypeInfo<Function<R(Args...)>> info;
  return info();
}

template<typename R, typename... Args>
Function<R(Args...)>::operator bool() const
{
  return _function;
}

namespace internal {

template<typename T>
struct TypeInfo {
  static_assert(sizeof(T) != sizeof(T), "use of unsupported type");

  yang::Type operator()() const
  {
    yang::Type t;
    return t;
  }
};

template<>
struct TypeInfo<void> {
  yang::Type operator()() const
  {
    yang::Type t;
    return t;
  }
};

template<>
struct TypeInfo<yang::int32> {
  yang::Type operator()() const
  {
    yang::Type t;
    t._base = yang::Type::INT;
    return t;
  }
};

template<>
struct TypeInfo<yang::world> {
  yang::Type operator()() const
  {
    yang::Type t;
    t._base = yang::Type::WORLD;
    return t;
  }
};

template<y::size N>
struct TypeInfo<int32_vec<N>> {
  yang::Type operator()() const
  {
    yang::Type t;
    t._base = yang::Type::INT;
    t._count = N;
    return t;
  }
};

template<y::size N>
struct TypeInfo<world_vec<N>> {
  yang::Type operator()() const
  {
    yang::Type t;
    t._base = yang::Type::WORLD;
    t._count = N;
    return t;
  }
};

// Templating TypeInfo on Function<R(Args...)> leads to ugly code, e.g. to
// retrieve a Function object for a function of Yang type int()():
//
//   auto f = instance.get_function<Function<Function<yang::int32()>()>>("f");
//
// It's tempting to think we could template simply on R(Args...) instead, but
// that leads to the syntax:
//
//   auto f = instance.get_function<yang::int32()()>("f");
//
// which, since C++ functions can only return *pointers* to other functions, is
// unfortunately illegal. For now, the only option right now is judicious use
// of typedefs (and perhaps a shorter typedef for Function).
template<typename R>
struct TypeInfo<Function<R()>> {
  yang::Type operator()() const
  {
    TypeInfo<R> return_type;

    yang::Type t;
    t._base = yang::Type::FUNCTION;
    t._elements.push_back(return_type());
    return t;
  }
};

template<typename R, typename A, typename... Args>
struct TypeInfo<Function<R(A, Args...)>> {
  yang::Type operator()() const
  {
    TypeInfo<Function<R(Args...)>> first;
    TypeInfo<A> arg_type;

    yang::Type t = first();
    t._elements.insert(++t._elements.begin(), arg_type());
    return t;
  }
};

template<typename T>
struct ValueConstruct {
  T operator()(Instance& instance) const
  {
    (void)instance;
    return T();
  }

  template<typename U>
  void set_void_fp(U&, y::void_fp ptr) const
  {
    static_assert(sizeof(U) != sizeof(U), "use of non-function type");
  }
};

template<typename R, typename... Args>
struct ValueConstruct<Function<R(Args...)>> {
  Function<R(Args...)> operator()(Instance& instance) const
  {
    return Function<R(Args...)>(instance);
  }

  void set_void_fp(Function<R(Args...)>& function, y::void_fp ptr) const
  {
    function._function = ptr;
  }
};

// Standard bind function requires explicit placeholders for all elements, which
// makes it unusable in the variadic setting. We only need to bind one argument
// at a time.
template<typename R, typename A, typename... Args>
y::function<R(Args...)> bind_first(
    const y::function<R(A, Args...)>& function, const A& arg)
{
  // Close by value so the function object is copied! Otherwise it can cause an
  // infinite loop. I don't fully understand why; is the function object being
  // mutated later somehow? We also need to copy the argument, as it may be a
  // temporary that won't last (particularly for return value pointers).
  return [function, arg](const Args&... args)
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
template<typename... Args>
struct Functions<List<Args...>> {
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
template<typename R, typename... Args>
struct TrampolineReturn<Function<R(Args...)>> {
  typedef List<y::void_fp*> type;
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
template<typename FR, typename... FArgs, typename... Args>
struct TrampolineArgs<Function<FR(FArgs...)>, Args...> {
  typedef typename Join<
      void (*)(), typename TrampolineArgs<Args...>::type>::type type;
};

template<typename R, typename... Args>
struct TrampolineType {
  typedef typename Join<
      typename TrampolineReturn<R>::type,
      typename TrampolineArgs<Args...>::type>::type type;
  typedef typename Functions<type>::fp_type fp_type;
  typedef typename Functions<type>::f_type f_type;
};

// Function call instrumentation for converting native types to the Yang calling
// convention for trampoline functions. TrampolineCallArgs unpacks the argument
// list; TrampolineCallReturn unpacks the return value.
template<typename... Args>
struct TrampolineCallArgs {};

// TrampolineCallArgs base case.
template<>
struct TrampolineCallArgs<> {
  typedef typename TrampolineType<void>::f_type f_type;

  void operator()(const f_type& function) const
  {
    function();
  }
};

// TrampolineCallArgs unpacking of a single primitive.
template<typename A, typename... Args>
struct TrampolineCallArgs<A, Args...> {
  typedef typename TrampolineType<void, A, Args...>::f_type f_type;

  void operator()(
      const f_type& function, const A& arg, const Args&... args) const
  {
    typedef TrampolineCallArgs<Args...> next_type;
    next_type()(bind_first(function, arg), args...);
  }
};

// TrampolineCallArgs unpacking of a vector.
template<typename T, y::size N, typename... Args>
struct TrampolineCallVecArgs {
  typedef typename TrampolineType<void, vec<T, N>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  template<y::size M, typename y::enable_if<(M >= N), bool>::type = 0>
  bound_f_type operator()(const f_type& function, const vec<T, M>& arg)
  {
    typedef TrampolineCallVecArgs<T, N - 1, T, Args...> first;
    auto f = first()(function, arg);
    return bind_first(f, arg[N - 1]);
  }
};
template<typename T, typename... Args>
struct TrampolineCallVecArgs<T, 2, Args...> {
  typedef typename TrampolineType<void, vec<T, 2>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  template<y::size M, typename y::enable_if<(M >= 2), bool>::type = 0>
  bound_f_type operator()(const f_type& function, const vec<T, M>& arg)
  {
    return bind_first(bind_first(function, arg[0]), arg[1]);
  }
};
template<typename T, y::size N, typename... Args>
struct TrampolineCallArgs<vec<T, N>, Args...> {
  typedef typename TrampolineType<void, vec<T, N>, Args...>::f_type f_type;

  void operator()(
      const f_type& function, const vec<T, N>& arg, const Args&... args) const
  {
    typedef TrampolineCallVecArgs<T, N, Args...> args_type;
    typedef TrampolineCallArgs<Args...> next_type;
    next_type()(args_type()(function, arg), args...);
  }
};

// TrampolineCallArgs unpacking of a function.
template<typename FR, typename... FArgs, typename... Args>
struct TrampolineCallArgs<Function<FR(FArgs...)>, Args...> {
  typedef typename TrampolineType<
      void, Function<FR(FArgs...)>, Args...>::f_type f_type;

  void operator()(
      const f_type& function, const Function<FR(FArgs...)>& arg,
      const Args&... args) const
  {
    typedef TrampolineCallArgs<Args...> next_type;
    next_type()(bind_first(function, arg._function), args...);
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

  bound_f_type operator()(const f_type& function, T* result) const
  {
    typedef TrampolineCallVecReturn<T, N - 1, T*, Args...> first;
    auto f = first()(function, result);
    return bind_first(f, (N - 1) + result);
  }
};
template<typename T, typename... Args>
struct TrampolineCallVecReturn<T, 2, Args...> {
  typedef typename TrampolineType<vec<T, 2>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, T* result) const
  {
    return bind_first(bind_first(function, result), 1 + result);
  }
};
template<typename T, y::size N, typename... Args>
struct TrampolineCallReturn<vec<T, N>, Args...> {
  typedef typename TrampolineType<vec<T, N>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(const f_type& function, vec<T, N>& result) const
  {
    typedef TrampolineCallVecReturn<T, N, Args...> return_type;
    return return_type()(function, result.elements);
  }
};

// TrampolineCallReturn for a function return.
template<typename FR, typename... FArgs, typename... Args>
struct TrampolineCallReturn<Function<FR(FArgs...)>, Args...> {
  typedef typename TrampolineType<
      Function<FR(FArgs...)>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  bound_f_type operator()(
      const f_type& function, Function<FR(FArgs...)>& result) const
  {
    return bind_first(function, &result._function);
  }
};

// Combine the above into a complete function call.
template<typename R, typename... Args>
struct TrampolineCall {
  typedef typename TrampolineType<R, Args...>::fp_type fp_type;
  typedef typename TrampolineType<R, Args...>::f_type f_type;

  R operator()(Instance& instance,
               const f_type& function, const Args&... args) const
  {
    typedef ValueConstruct<R> construct_type;
    typedef TrampolineCallReturn<R, Args...> return_type;
    typedef TrampolineCallArgs<Args...> args_type;

    R result = construct_type()(instance);
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

  void operator()(Instance& instance,
                  const f_type& function, const Args&... args) const
  {
    (void)instance;
    typedef TrampolineCallArgs<Args...> args_type;
    args_type()(function, args...);
  }
};

// End namespace yang::internal.
}
}

#endif
