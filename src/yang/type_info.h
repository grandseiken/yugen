#ifndef YANG__TYPE_INFO_H
#define YANG__TYPE_INFO_H

#include "../common/function.h"
#include "../common/math.h"
#include "../common/tuple.h"
#include "../vector.h"
#include "native_function.h"
#include "type.h"

namespace yang {

class Instance;
namespace internal {
  template<typename>
  struct TypeInfo;
  template<typename>
  struct ValueConstruct;

  template<typename...>
  struct TrampolineCallArgs;
  template<typename, typename...>
  struct TrampolineCallReturn;
  template<typename, typename...>
  struct TrampolineCall;

  template<typename, typename, typename>
  struct ReverseTrampolineCallArgs;
  template<typename, typename...>
  struct ReverseTrampolineCallReturn;
}

typedef y::int32 int32;
typedef y::world world;
template<typename T, y::size N, typename = y::enable_if<(N > 1)>>
using vec = y::vec<T, N>;
template<y::size N, typename = y::enable_if<(N > 1)>>
using int32_vec = y::vec<int32, N>;
template<y::size N, typename = y::enable_if<(N > 1)>>
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

  template<typename...>
  friend struct internal::TrampolineCallArgs;
  template<typename, typename...>
  friend struct internal::TrampolineCallReturn;
  template<typename, typename...>
  friend struct internal::TrampolineCall;

  template<typename, typename, typename>
  friend struct ReverseTrampolineCallArgs;
  template<typename, typename...>
  friend struct ReverseTrampolineCallReturn;

  template<typename>
  friend struct internal::ValueConstruct;
  template<typename>
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
    // This function exists to avoid extra unnecessary error messages (beyond
    // static assert above).
    return {};
  }
};

template<>
struct TypeInfo<void> {
  yang::Type operator()() const
  {
    return {};
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
  T operator()(Instance&) const
  {
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

// Boost's metaprogramming list implementation fakes variadic templates for
// C++03 compatibility. This makes extracting the type-list directly as a
// comma-separated type list for constructing e.g. function pointer types
// difficult.
template<typename... T>
using List = y::tuple<T...>;
// Some function metadata.
template<typename R, typename... Args>
struct Functions {
  typedef R (*fp_type)(Args...);
  typedef y::function<R(Args...)> f_type;
};
template<typename R, typename... Args>
struct Functions<R, List<Args...>> {
  typedef typename Functions<R, Args...>::fp_type fp_type;
  typedef typename Functions<R, Args...>::f_type f_type;
};

// Integer pack range.
template<y::size... N>
struct Indices {};
template<y::size Min, y::size N, y::size... M>
struct IndexRange {
  typedef typename IndexRange<Min, N - 1, Min + N - 1, M...>::type type;
};
template<y::size Min, y::size... N>
struct IndexRange<Min, 0, N...> {
  typedef Indices<N...> type;
};

// Join two lists.
template<typename T, typename U>
struct Join {};
template<typename... T, typename... U>
struct Join<List<T...>, List<U...>> {
  typedef List<T..., U...> type;
};

// Get a sublist from a list.
template<typename, typename>
struct Sublist {};
template<y::size... N, typename... T>
struct Sublist<Indices<N...>, List<T...>> {
  typedef List<typename y::tuple_elem<N, List<T...>>::type...> type;

  type operator()(const List<T...>& t) const
  {
    return type(y::get<N>(t)...);
  }
};

// Standard bind function requires explicit placeholders for all elements, which
// makes it unusable in the variadic setting. We only need to bind one argument
// at a time.
template<typename, typename, typename>
struct BindFirstHelperInternal {};
template<typename R, typename... Args, typename... Brgs>
struct BindFirstHelperInternal<R, List<Args...>, List<Brgs...>> {
  template<y::size... N>
  y::function<R(Brgs...)> operator()(
      const y::function<R(Args..., Brgs...)>& function,
      const Args&... args, const Indices<N...>&) const
  {
    // Work around bug in GCC/C++ standard: ambiguous whether argument packs can
    // be captured.
    List<Args...> args_tuple(args...);
    // Close by value so the function object is copied! Otherwise it can cause an
    // infinite loop. I don't fully understand why; is the function object being
    // mutated later somehow? We also need to copy the argument, as it may be a
    // temporary that won't last (particularly for return value pointers).
    return [=](const Brgs&... brgs)
    {
      return function(y::get<N>(args_tuple)..., brgs...);
    };
  }
};
// Wrapper stuff to make deducing template arguments at all possible.
template<typename>
struct BindFirstHelper {};
template<typename R, typename... Args>
struct BindFirstHelper<R(Args...)> {
  template<typename... Brgs>
  y::function<R(Brgs...)> operator()(
      const y::function<R(Args..., Brgs...)> function,
      const Args&... args) const
  {
    return BindFirstHelperInternal<R, List<Args...>, List<Brgs...>>()(
        function, args..., typename IndexRange<0, sizeof...(Args)>::type());
  }
};
template<typename R, typename... Brgs, typename... Args>
auto bind_first(const y::function<R(Brgs...)>& function, const Args&... args)
  -> decltype(BindFirstHelper<R(Args...)>()(function, args...))
{
  return BindFirstHelper<R(Args...)>()(function, args...);
}


// Templates for converting native function types into the corresponding
// Yang trampoline function types.
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
      List<T*>, typename TrampolineReturn<vec<T, N - 1>>::type>::type type;
};
template<typename R, typename... Args>
struct TrampolineReturn<Function<R(Args...)>> {
  typedef List<y::void_fp*> type;
};

template<typename... Args>
struct TrampolineArgs {};
template<typename A, typename... Args>
struct TrampolineArgs<A, Args...> {
  typedef typename Join<
      List<A>, typename TrampolineArgs<Args...>::type>::type type;
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
template<typename R, typename... Args, typename... Brgs>
struct TrampolineArgs<Function<R(Args...)>, Brgs...> {
  typedef typename Join<
      List<y::void_fp>, typename TrampolineArgs<Brgs...>::type>::type type;
};

template<typename R, typename... Args>
struct TrampolineType {
  typedef typename Join<
      typename TrampolineReturn<R>::type,
      typename TrampolineArgs<Args...>::type>::type type;
  typedef typename Functions<void, type>::fp_type fp_type;
  typedef typename Functions<void, type>::f_type f_type;
};

// Function call instrumentation for converting native types to the Yang calling
// convention for trampoline functions. TrampolineCallArgs unpacks the argument
// list; TrampolineCallReturn unpacks the return value.
template<typename...>
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
    TrampolineCallArgs<Args...>()(bind_first(function, arg), args...);
  }
};

// TrampolineCallArgs unpacking of a vector.
template<typename T, y::size N, typename... Args>
struct TrampolineCallArgs<vec<T, N>, Args...> {
  typedef typename TrampolineType<void, vec<T, N>, Args...>::f_type f_type;
  typedef typename TrampolineType<void, Args...>::f_type bound_f_type;

  template<y::size... M>
  bound_f_type helper(
      const f_type& function, const vec<T, N>& arg, const Indices<M...>&) const
  {
    return bind_first(function, arg[M]...);
  }                

  void operator()(
      const f_type& function, const vec<T, N>& arg, const Args&... args) const
  {
    auto f = helper(function, arg, typename IndexRange<0, N>::type());
    TrampolineCallArgs<Args...>()(f, args...);
  }
};

// TrampolineCallArgs unpacking of a function.
template<typename R, typename... Args, typename... Brgs>
struct TrampolineCallArgs<Function<R(Args...)>, Brgs...> {
  typedef typename TrampolineType<
      void, Function<R(Args...)>, Brgs...>::f_type f_type;

  void operator()(
      const f_type& function, const Function<R(Args...)>& arg,
      const Brgs&... args) const
  {
    TrampolineCallArgs<Brgs...>()(bind_first(function, arg._function), args...);
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
    auto f = TrampolineCallVecReturn<T, N - 1, T*, Args...>()(function, result);
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
    return TrampolineCallVecReturn<T, N, Args...>()(function, result.elements);
  }
};

// TrampolineCallReturn for a function return.
template<typename R, typename... Args, typename... Brgs>
struct TrampolineCallReturn<Function<R(Args...)>, Brgs...> {
  typedef typename TrampolineType<
      Function<R(Args...)>, Brgs...>::f_type f_type;
  typedef typename TrampolineType<void, Brgs...>::f_type bound_f_type;

  bound_f_type operator()(
      const f_type& function, Function<R(Args...)>& result) const
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

  void operator()(Instance&,
                  const f_type& function, const Args&... args) const
  {
    typedef TrampolineCallArgs<Args...> args_type;
    args_type()(function, args...);
  }
};

// Function call instrumentation for reverse trampoline conversion to native
// calling convention. ReverseTrampolineCallArgs corresponds to
// TrampolineCallArgs, and so on.
template<typename, typename, typename>
struct ReverseTrampolineCallArgs {};
template<typename R>
struct ReverseTrampolineCallArgs<R, List<>, List<>> {
  R operator()(const List<>&, void* global_data,
               const y::function<R()>& target) const
  {
    return target();
  }
};
template<typename R, typename... Args, typename... Brgs, typename A>
struct ReverseTrampolineCallArgs<R, List<A, Args...>, List<Brgs...>> {
  R operator()(const List<Brgs...>& args, void* global_data,
               const y::function<R(A, Args...)>& target) const
  {
//    typedef ReverseTrampolineCallArgs<
//        R, List<Args...>, typename Remove<List<Brgs...>, 1>::type> next;
//    next()(args..., bind_first(target, a));
    return R();
  }
};
template<typename R, typename... Args, typename... Brgs,
         typename T, y::size N>
struct ReverseTrampolineCallArgs<R, List<vec<T, N>, Args...>,
                                 List<Brgs...>> {
  R operator()(const Brgs&... args, void* global_data,
               const y::function<R(vec<T, N>, Args...)>& target) const
  {
    // TODO: make this do anything useful. Need to split off the first N from
    // the args list. Consider using Remove and that for all of these.
    typedef ReverseTrampolineCallArgs<R, List<Args...>, List<Brgs...>> next;
    next()(args..., bind_first(target, vec<T, N>()));
  }
};
template<typename R, typename... Args, typename... Brgs,
         typename S, typename... Crgs>
struct ReverseTrampolineCallArgs<R, List<Function<S(Crgs...)>, Args...>,
                                 List<y::void_fp, Brgs...>> {
  R operator()(
      y::void_fp a, const Brgs&... args, void* global_data,
      const y::function<R(Function<S(Crgs...)>, Args...)>& target) const
  {
    typedef ReverseTrampolineCallArgs<R, List<Args...>, List<Brgs...>> next;
    // TODO: global data needs to store a pointer to the instance! This is
    // totally wrong. Can get rid of the default single integer, though.
    Function<S(Crgs...)> fn_object(*(Instance*)(global_data));
    fn_object._function = a;
    next()(args..., bind_first(target, fn_object));
  }
};

// Return-value assigned to pointer outputs.
template<typename, typename...>
struct ReverseTrampolineCallReturn {};
template<typename R>
struct ReverseTrampolineCallReturn<R, R*> {
  void operator()(const R& result, R* ptr) const
  {
    *ptr = result;
  }
};
template<typename T, y::size N, typename... Args>
struct ReverseTrampolineCallVecReturn {};
template<typename T, y::size N, typename... Args>
struct ReverseTrampolineCallVecReturn<T, N, T*, Args...> {
  template<y::size M, typename = y::enable_if<(M >= N)>>
  void operator()(const vec<T, M>& result, const Args&... args, T* a) const
  {
    typedef ReverseTrampolineCallVecReturn<T, N - 1, Args...> next;
    *a = result[N - 1];
    next()(result, args...);
  }
};
template<typename T>
struct ReverseTrampolineCallVecReturn<T, 2, T*, T*> {
  template<y::size M, typename = y::enable_if<(M >= 2)>>
  void operator()(const vec<T, M>& result, T* a0, T* a1) const
  {
    *a0 = result[0];
    *a1 = result[1];
  }
};
template<typename T, y::size N, typename... Args>
struct ReverseTrampolineCallReturn<vec<T, N>, Args...> {
  void operator()(const vec<T, N>& result, const Args&... args) const
  {
    typedef ReverseTrampolineCallVecReturn<T, N, Args...> return_type;
    return_type()(result, args...);
  }
};
template<typename R, typename... Args>
struct ReverseTrampolineCallReturn<Function<R(Args...)>, y::void_fp*> {
  void operator()(const Function<R(Args...)>& result, y::void_fp* ptr) const
  {
    *ptr = result._function;
  }
};

// Combine into reverse function call.
template<typename, typename, typename>
struct ReverseTrampolineCall {};

// For convenience, this template takes both the native template arguments R and
// Brgs, and their instantiation lists TrampolineReturn<R>::type and
// TrampolineArgs<Brgs>::type respectively.
template<typename R, typename... Args, typename... ReturnBrgs, typename... Brgs>
struct ReverseTrampolineCall<R(Args...), List<ReturnBrgs...>, List<Brgs...>> {
  // No reference args; this is the function directly called from Yang code.
  static void call(ReturnBrgs... return_args, Brgs... args,
                   void* global_data, void* target)
  {
    // TODO: need checking that functions returned to Yang reference the correct
    // module.
    typedef ReverseTrampolineCallReturn<R, ReturnBrgs...> return_type;
    typedef ReverseTrampolineCallArgs<
        R, List<Args...>, List<Brgs...>> args_type;

    auto f = (const NativeFunction<void>*)target;
    R result = args_type()(
        List<Brgs...>(args...), global_data, f->get<R, Args...>());
    return_type()(result, return_args...);
  }
};

// Specialisation for void returns.
template<typename... Args, typename... Brgs>
struct ReverseTrampolineCall<void(Args...), List<>, List<Brgs...>> {
  static void call(Brgs... args, void* global_data, void* target)
  {
    typedef ReverseTrampolineCallArgs<
        void, List<Args...>, List<Brgs...>> args_type;

    auto f = (const NativeFunction<void>*)target;
    args_type()(List<Brgs...>(args...), global_data, f->get<void, Args...>());
  }
};

// End namespace yang::internal.
}
}

#endif
